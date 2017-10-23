/**
 * @file EmulatedTestMain.cpp
 * A "main" file for running the smart metering protocol with a "real" network
 * (probably emulated within one machine) but simulated smart meter data. Each
 * process running this program will emulate one smart meter.
 * @date Oct 7, 2016
 * @author edward
 */

#include <cstdlib>
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
    std::cout <<" Result was: " << *result << std::endl;
}

int main(int argc, char** argv) {
    if(argc < 8) {
        std::cout << "Expected arguments: <my ID> <utility IP address> <meter IP configuration file> <device configuration files> " << std::endl;
        std::cout << "Device characteristic files are: power load, mean daily frequency, "
                "hourly usage probability, and household saturation." << std::endl;
        return -1;
    }

    //Set up static global logging framework
    auto logger = spdlog::rotating_logger_mt("global_logger", "simulation-log", 1024 * 1024 * 500, 3);
    logger->set_pattern("[%H:%M:%S.%e] [%l] %v");
    logger->set_level(spdlog::level::debug);

    //First argument is my ID
    int meter_id = std::atoi(argv[1]);

    //Second argument is assumed to be the utility's IP:port
    networking::TcpAddress utility_ip = networking::parse_tcp_string(std::string(argv[2]));

    //Read and parse the IP addresses
    std::map<int, networking::TcpAddress> meter_ips_by_id = util::read_ip_map_from_file(std::string(argv[3]));

    int num_meters = meter_ips_by_id.size();
    int modulus = util::get_valid_prime_modulus(num_meters);
    if(modulus != num_meters) {
        std::cout << "ERROR: The number of meters specified in " << std::string(argv[3]) << " is not a valid prime. "
                << "This experiment does not handle non-prime numbers of meters." << std::endl;
        return -1;
    }
    ProtocolState_t::init_failures_tolerated(num_meters);

    const int NUM_QUERIES = 10;
    const auto TIME_PER_TIMESTEP = std::chrono::seconds(30);
    //An ID of -1 means this should be the utility client, not a meter client
    if(meter_id < 0) {
        auto utility_client = std::make_unique<UtilityClient>(num_meters,
                networking::utility_network_client_builder(utility_ip, meter_ips_by_id),
                util::crypto_library_builder_utility(),
                util::timer_manager_builder_utility());
        utility_client->register_query_callback(query_finished_callback);
        for(int query_count = 0; query_count < NUM_QUERIES; ++query_count) {
            auto query_req = std::make_shared<messaging::QueryRequest>(messaging::QueryType::CURR_USAGE_SUM, 30, query_count);
            std::cout << "Starting query " << query_count << std::endl;
            utility_client->start_query(query_req);
            int timesteps_in_30_min = 30 / simulation::USAGE_TIMESTEP_MIN;
            std::this_thread::sleep_for(TIME_PER_TIMESTEP * timesteps_in_30_min);
        }
        std::cout << "Done issuing queries, entering infinite loop" << std::endl;
        while(true) {
        };
    } else {
        networking::TcpAddress my_ip = meter_ips_by_id.at(meter_id);
        std::map<std::string, simulation::Device> possible_devices;
        std::map<std::string, double> devices_saturation;
        util::read_devices_from_files(std::string(argv[4]), std::string(argv[5]),
                std::string(argv[6]), std::string(argv[7]),
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
                util::crypto_library_builder(),
                util::timer_manager_builder());

        //Start a background thread to repeatedly poke the simulated meter
        std::thread sim_meter_advance_thread([TIME_PER_TIMESTEP, sim_meter]() {
            while(true) {
                std::this_thread::sleep_for(TIME_PER_TIMESTEP);
                sim_meter->simulate_usage_timestep();
                std::cout << "Advanced simulated meter by " << simulation::USAGE_TIMESTEP_MIN << " minutes" << std::endl;
            }
        });

        //Start waiting for incoming messages to respond to. This will not return.
        my_client->main_loop();
    }
    return 0;
}
