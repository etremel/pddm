/**
 * @file EmulatedTestWithCrypto.cpp
 *
 * @date Dec 12, 2017
 * @author edward
 */


#include <iostream>
#include <map>
#include <random>
#include <spdlog/spdlog.h>

#include "MeterClient.h"
#include "UtilityClient.h"
#include "networking/TcpAddress.h"
#include "util/ConfigParser.h"
#include "simulation/SimParameters.h"

using namespace pddm;

void query_finished_callback(const int query_num, std::shared_ptr<messaging::AggregationMessageValue> result) {
    std::cout << "Query " << query_num << " finished." << std::endl;
    std::cout << "Result was: " << *result << std::endl;
}

void print_usage_info() {
    std::cout << "Usage:" << std::endl;
    std::cout << "For utility-client mode, arguments are: -1 <my IP> <utility private key file> <meter IP configuration file> <public key folder>" << std::endl << std::endl;;
    std::cout << "For meter-client mode, arguments are: <my ID> <utility IP address> <utility public key file> "
            << "<meter IP configuration file> <public key folder> <private key folder> <device configuration files> " << std::endl;
    std::cout << "Device characteristic files are: power load, mean daily frequency, "
            "hourly usage probability, and household saturation." << std::endl;
}

int main(int argc, char** argv) {
    const int NUM_QUERIES = 3;
    const auto TIME_PER_TIMESTEP = std::chrono::seconds(10);

    if(argc < 6) {
        std::cout << "Too few arguments!" << std::endl;
        print_usage_info();
        return -1;
    }

    int meter_id = std::atoi(argv[1]);

    //Set up static global logging framework
    std::string filename("emulated_log_node_" + std::to_string(meter_id) + ".txt");
    std::vector<spdlog::sink_ptr> log_sinks;
    log_sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, 1024 * 1024 * 500, 3));
    log_sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    std::shared_ptr<spdlog::logger> logger = spdlog::create("global_logger", log_sinks.begin(), log_sinks.end());
    logger->set_pattern("[%H:%M:%S.%e] [%l] %v");
    logger->set_level(spdlog::level::debug);

    //Second argument is always the utility's IP:port
    networking::TcpAddress utility_ip = networking::parse_tcp_string(std::string(argv[2]));

    //Fourth argument is always the meter IP addresses file
    std::map<int, networking::TcpAddress> meter_ips_by_id = util::read_ip_map_from_file(std::string(argv[4]));

    int num_meters = meter_ips_by_id.size();
    int modulus = util::get_valid_prime_modulus(num_meters);
    if(modulus != num_meters) {
        std::cout << "ERROR: The number of meters specified in " << std::string(argv[4]) << " is not a valid prime. "
                << "This experiment does not handle non-prime numbers of meters." << std::endl;
        return -1;
    }
    ProtocolState_t::init_failures_tolerated(num_meters);

    //Build the file paths for all the public keys based on the folder name
    std::string meter_public_key_folder = std::string(argv[5]);
    std::map<int, std::string> meter_public_key_files;
    for(int id = 0; id < num_meters; ++id) {
        std::stringstream file_path;
        file_path << meter_public_key_folder << "/pubkey_" << id << ".der";
        meter_public_key_files.emplace(id, file_path.str());
    }

    //Now branch based on first argument (ID); if -1, we're the utility client
    if(meter_id == -1) {
        std::string utility_private_key_file = std::string(argv[3]);

        auto utility_client = std::make_unique<UtilityClient>(num_meters,
                networking::utility_network_client_builder(utility_ip, meter_ips_by_id),
                util::crypto_library_builder_utility(utility_private_key_file, meter_public_key_files),
                util::timer_manager_builder_utility());
        utility_client->register_query_callback(query_finished_callback);

        //Start a background thread to issue queries
        std::thread utility_query_thread([NUM_QUERIES, TIME_PER_TIMESTEP, logger, &utility_client]() {
            for(int query_count = 0; query_count < NUM_QUERIES; ++query_count) {
                auto query_req = std::make_shared<messaging::QueryRequest>(messaging::QueryType::CURR_USAGE_SUM, 30, query_count);
                std::cout << "Starting query " << query_count << std::endl;
                logger->info("Utility starting query {}", query_count);
                utility_client->start_query(query_req);
                int timesteps_in_30_min = 30 / simulation::USAGE_TIMESTEP_MIN;
                std::this_thread::sleep_for(TIME_PER_TIMESTEP * timesteps_in_30_min);
            }
            std::cout << "Done issuing queries" << std::endl;
            std::this_thread::sleep_for(TIME_PER_TIMESTEP * 3);
            utility_client->shut_down();
        });
        //Start listening for incoming messages. This will not return.
        utility_client->listen_loop();
        utility_query_thread.join();
    } else {
        std::string utility_public_key_file = std::string(argv[3]);
        meter_public_key_files.emplace(UTILITY_NODE_ID, utility_public_key_file);
        if(argc < 11) {
            std::cout << "Too few arguments!" << std::endl;
            print_usage_info();
            return -1;
        }
        std::string meter_private_key_folder = std::string(argv[6]);
        std::stringstream private_key_path;
        private_key_path << meter_private_key_folder << "/privkey_" << meter_id << ".der";

        networking::TcpAddress my_ip = meter_ips_by_id.at(meter_id);
        std::map<std::string, simulation::Device> possible_devices;
        std::map<std::string, double> devices_saturation;
        util::read_devices_from_files(std::string(argv[7]), std::string(argv[8]),
                std::string(argv[9]), std::string(argv[10]),
                possible_devices, devices_saturation);

        //For now I'll just hard-code this in, since I'm not paying attention to prices
        PriceFunction sim_energy_price = [](const int time_of_day) {
            if(time_of_day > 17 && time_of_day < 20) {
                return util::Money(0.0734);
            } else {
                return util::Money(0.0612);
            }
        };
        std::mt19937 random_engine;
        std::discrete_distribution<> income_distribution({25, 50, 25});
        //Generate a meter with simulated devices, but put it in a shared_ptr because that's what MeterClient expects
        std::shared_ptr<simulation::Meter> sim_meter(simulation::generate_meter(income_distribution, possible_devices,
                devices_saturation, sim_energy_price, random_engine).release());


        auto my_client = std::make_unique<MeterClient>(meter_id, num_meters, sim_meter,
                networking::network_client_builder(my_ip, utility_ip, meter_ips_by_id),
                util::crypto_library_builder(private_key_path.str(), meter_public_key_files),
                util::timer_manager_builder());

        //Start a background thread to repeatedly poke the simulated meter
        std::thread sim_meter_advance_thread([TIME_PER_TIMESTEP, sim_meter]() {
            for(int timestep = 0; timestep < simulation::TOTAL_TIMESTEPS; ++timestep) {
                sim_meter->simulate_usage_timestep();
                std::cout << "Advanced simulated meter by " << simulation::USAGE_TIMESTEP_MIN << " minutes" << std::endl;
                std::this_thread::sleep_for(TIME_PER_TIMESTEP);
            }
        });

        //Start waiting for incoming messages to respond to. This will not return.
        my_client->main_loop();
        sim_meter_advance_thread.join();
    }

    return 0;
}

