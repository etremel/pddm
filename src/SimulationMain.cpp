/**
 * @file SimulationMain.cpp
 * Entry point for running the smart metering system in simulation mode.
 * @date Aug 9, 2016
 * @author edward
 */

#include <string>
#include <set>
#include <iostream>

#include "simulation/Simulator.h"
#include "spdlog/spdlog.h"

using namespace pddm;

void measure_query_run_times(char** argv) {
    std::vector<int> grid_sizes = {101, 197, 419, 613,  797, 1019, 1997, 3011, 5003};
    std::vector<int> failure_nums = {10, 19, 41, 61, 79, 101, 199, 301, 500};

    for(int i = 0; i < grid_sizes.size(); ++i) {
        simulation::Simulator sim;
        sim.setup_simulation(grid_sizes[i], std::string(argv[1]),
                std::string(argv[2]), std::string(argv[3]), std::string(argv[4]));
        sim.run(std::set<simulation::QueryMode>{simulation::QueryMode::ONLY_ONE_QUERY});
    }
}


int main(int argc, char** argv) {
    if(argc < 5) {
        std::cout << "You must provide 4 data files for the characteristics of the appliances: " <<
                "power load, mean daily frequency, hourly usage probability, and household saturation." << std::endl;
        return -1;
    }

    //Set up static global logging framework
    auto logger = spdlog::rotating_logger_mt("global_logger", "simulation-log", 1024 * 1024 * 500, 3);
    logger->set_pattern("[%H:%M:%S.%e] [%l] %v");
    logger->set_level(spdlog::level::debug);

    measure_query_run_times(argv);
//    const int num_homes = 197;
//    simulation::Simulator sim;
//    sim.setup_simulation(num_homes, std::string(argv[1]),
//            std::string(argv[2]), std::string(argv[3]), std::string(argv[4]));
//    sim.run(std::set<simulation::QueryMode>{simulation::QueryMode::ONLY_ONE_QUERY});
}

