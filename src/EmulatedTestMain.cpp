/**
 * @file EmulatedTestMain.cpp
 * A "main" file for running the smart metering protocol with a "real" network
 * (probably emulated within one machine) but simulated smart meter data. Each
 * process running this program will emulate one smart meter.
 * @date Oct 7, 2016
 * @author edward
 */


#include "MeterClient.h"
#include "UtilityClient.h"

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

}
