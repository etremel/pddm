/**
 * @file ConfigParser.cpp
 *
 * @date Apr 13, 2017
 * @author edward
 */

#include "ConfigParser.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

#include "../FixedPoint_t.h"

using std::string;

namespace pddm {
namespace util {

void read_devices_from_files(const std::string& device_power_data_file, const std::string& device_frequency_data_file,
        const std::string& device_probability_data_file, const std::string& device_saturation_data_file,
        std::map<std::string, simulation::Device>& possible_devices, std::map<std::string, double>& devices_saturation) {
    std::ifstream power_data_stream(device_power_data_file);
    std::ifstream frequency_data_stream(device_frequency_data_file);
    std::ifstream hourly_probability_stream(device_probability_data_file);
    std::ifstream saturation_stream(device_saturation_data_file);

    using namespace simulation;
    string line;
    while(std::getline(power_data_stream, line)) {
        std::istringstream power_data(line);
        //Text until the first tab is the device's name
        string name;
        std::getline(power_data, name, '\t'); //Deceptive name: this does not read a "line," just a single tab-delimited field
        //Now that we have a name, we can construct the device
        possible_devices.emplace(name, Device{});
        Device& cur_device = possible_devices.at(name);
        cur_device.name = name;
        //The rest of the line is tab-separated numbers representing power usage cycles, followed by the standby load
        //Fortunately, the default delimiter for istream's operator>> is "any whitespace"
        //Unfortunately, the only way to use istream_iterator is to read the whole line, including the standby load, into one vector
        std::vector<int> mixed_cycle_data{std::istream_iterator<int>{power_data}, std::istream_iterator<int>{}};
        //Take the standby load value off the end of the vector, leaving just the cycle data
        cur_device.standby_load = FixedPoint_t{static_cast<double>(mixed_cycle_data.back())};
        mixed_cycle_data.pop_back();
        //Cycle data alternates between load per cycle and minutes per cycle
        cur_device.load_per_cycle.resize(mixed_cycle_data.size()/2);
        cur_device.time_per_cycle.resize(cur_device.load_per_cycle.size());
        for(size_t i = 0; i < mixed_cycle_data.size(); i += 2) {
            cur_device.load_per_cycle[i/2] = FixedPoint_t(static_cast<double>(mixed_cycle_data[i]));
            cur_device.time_per_cycle[i/2] = mixed_cycle_data[i+1];
        }
    }

    while(std::getline(frequency_data_stream, line)) {
        std::istringstream frequency_data(line);
        //All text until the first tab is the name, then there are two tab-delimited doubles
        string name;
        std::getline(frequency_data, name, '\t');
        frequency_data >> possible_devices.at(name).weekday_frequency;
        frequency_data >> possible_devices.at(name).weekend_frequency;
    }

    while(std::getline(hourly_probability_stream, line)) {
        std::istringstream hourly_prob_data(line);
        string name;
        std::getline(hourly_prob_data, name, '\t');
        Device& cur_device = possible_devices.at(name);
        string series_type;
        hourly_prob_data >> series_type;
        if(series_type == "we") {
            cur_device.weekend_hourly_probability.resize(24);
            //"Extract all elements of hourly_prob_data as doubles, divide them by 100, and copy them to weekend_hourly_probability"
            //(For some reason, the probability factors are given as percentages in the config file)
            std::transform(std::istream_iterator<double>(hourly_prob_data), std::istream_iterator<double>(),
                    cur_device.weekend_hourly_probability.begin(),
                    [](const double prob_data_element){ return prob_data_element/100.0; });
        } else {
            cur_device.weekday_hourly_probability.resize(24);
            std::transform(std::istream_iterator<double>(hourly_prob_data), std::istream_iterator<double>(),
                    cur_device.weekday_hourly_probability.begin(),
                    [](const double prob_data_element){ return prob_data_element/100.0; });
        }
    }

    while(std::getline(saturation_stream, line)) {
        std::istringstream saturation_data(line);
        string name;
        std::getline(saturation_data, name, '\t');
        double saturation;
        saturation_data >> saturation;
        devices_saturation[name] = saturation;
    }
}

std::map<int, networking::TcpAddress> read_ip_map_from_file(const std::string& meter_ips_file) {
    std::map<int, networking::TcpAddress> meter_ips_by_id;

    std::ifstream ip_table_stream(meter_ips_file);
    string line;
    while(std::getline(ip_table_stream, line)) {
        std::istringstream id_ip_entry(line);
        int meter_id;
        std::string ip_address_string;
        //ID and IP address are just separated by a space, so we can use the standard stream extractor
        id_ip_entry >> meter_id;
        id_ip_entry >> ip_address_string;
        meter_ips_by_id.emplace(meter_id, networking::parse_tcp_string(ip_address_string));
    }

    return meter_ips_by_id;
}

}
}


