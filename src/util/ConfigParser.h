/**
 * @file ConfigParser.h
 *
 * @date Apr 13, 2017
 * @author edward
 */

#pragma once

#include <map>
#include <string>

#include "simulation/Device.h"
#include "../networking/TcpAddress.h"

namespace pddm {
namespace util {

/**
 * Parses a set of configuration files to get a set of possible devices for simulated meters.
 * Output is provided by filling the two maps passed in by non-const reference.
 */
void read_devices_from_files(const std::string& device_power_data_file, const std::string& device_frequency_data_file,
        const std::string& device_probability_data_file, const std::string& device_saturation_data_file,
        std::map<std::string, simulation::Device>& possible_devices, std::map<std::string, double>& devices_saturation);


/**
 * Parses a configuration file that contains a whitespace-separated table of
 * device IDs and IP addresses, and returns the corresponding map of device ID
 * to IP address.
 */
std::map<int, networking::TcpAddress> read_ip_map_from_file(const std::string& meter_ips_file);

} /* namespace util */
} /* namespace util */


