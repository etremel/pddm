/**
 * @file UtilityClientTest.cpp
 *
 * @date Nov 3, 2017
 * @author edward
 */

#include <memory>
#include <spdlog/spdlog.h>
#include <iostream>

#include "UtilityClient.h"
#include "networking/TcpAddress.h"
#include "util/ConfigParser.h"
#include "simulation/SimParameters.h"

using namespace pddm;

void query_finished_callback(const int query_num, std::shared_ptr<messaging::AggregationMessageValue> result) {
    std::cout << "Query " << query_num << " finished." << std::endl;
    std::cout << "Result was: " << *result << std::endl;
}

int main(int argc, char** argv) {
    if(argc < 3) {
        std::cout << "Expected arguments: <utility IP address> <meter IP configuration file> " << std::endl;
        return -1;
    }

    int meter_id = -1;

    //Set up static global logging framework
    std::string filename("emulated_log_node_" + std::to_string(meter_id));
    std::vector<spdlog::sink_ptr> log_sinks;
    log_sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, 1024 * 1024 * 500, 3));
    log_sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    std::shared_ptr<spdlog::logger> logger = spdlog::create("global_logger", log_sinks.begin(), log_sinks.end());
    logger->set_pattern("[%H:%M:%S.%e] [%l] %v");
    logger->set_level(spdlog::level::trace);

    //First argument is the utility's IP:port
    networking::TcpAddress utility_ip = networking::parse_tcp_string(std::string(argv[1]));

    //Read and parse the IP addresses
    std::map<int, networking::TcpAddress> meter_ips_by_id = util::read_ip_map_from_file(std::string(argv[2]));

    int num_meters = meter_ips_by_id.size();
    int modulus = util::get_valid_prime_modulus(num_meters);
    if(modulus != num_meters) {
        std::cout << "ERROR: The number of meters specified in " << std::string(argv[2]) << " is not a valid prime. "
                << "This experiment does not handle non-prime numbers of meters." << std::endl;
        return -1;
    }
    ProtocolState_t::init_failures_tolerated(num_meters);


    std::unique_ptr<UtilityClient> utility_client = std::make_unique<UtilityClient>(num_meters,
                networking::utility_network_client_builder(utility_ip, meter_ips_by_id),
                util::crypto_library_builder_utility(),
                util::timer_manager_builder_utility());
    utility_client->register_query_callback(query_finished_callback);
    std::thread shutdown_thread([&utility_client](){
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "Shutting down the foreground thread" << std::endl;
        utility_client->shut_down();
    });
    std::cout << "Going into listen loop" << std::endl;
    utility_client->listen_loop();
    shutdown_thread.join();
    return 0;
}
