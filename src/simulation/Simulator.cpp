/*
 * Simulator.cpp
 *
 *  Created on: May 27, 2016
 *      Author: edward
 */

#include "Simulator.h"

#include <stddef.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <list>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <memory>

#include "../Configuration.h"
#include "../messaging/QueryRequest.h"
#include "../util/Money.h"
#include "../util/Overlay.h"
#include "../UtilityClient.h"
#include "Event.h"
#include "IncomeLevel.h"
#include "Meter.h"
#include "Network.h"
#include "SimCrypto.h"
#include "SimCryptoWrapper.h"
#include "SimNetworkClient.h"
#include "SimParameters.h"
#include "SimTimerManager.h"
#include "SimUtilityNetworkClient.h"
#include "Timesteps.h"
#include "../ConfigurationIncludes.h"

using std::string;

namespace pddm {
namespace simulation {

void Simulator::read_devices_from_files(const string& device_power_data_file, const string& device_frequency_data_file,
        const string& device_probability_data_file, const string& device_saturation_data_file) {
    std::ifstream power_data_stream(device_power_data_file);
    std::ifstream frequency_data_stream(device_frequency_data_file);
    std::ifstream hourly_probability_stream(device_probability_data_file);
    std::ifstream saturation_stream(device_saturation_data_file);

    string line;
    while(std::getline(power_data_stream, line)) {
        std::istringstream power_data(line);
        //Text until the first tab is the device's name
        string name;
        std::getline(power_data, name, '\t'); //Deceptive name: this does not read a "line," just a single tab-delimited field
        //Now that we have a name, we can construct the device
        possible_devices.emplace(name, Device{});
        Device& cur_device = possible_devices[name];
        cur_device.name = name;
        //The rest of the line is tab-separated numbers representing power usage cycles, followed by the standby load
        //Fortunately, the default delimiter for istream's operator>> is "any whitespace"
        //Unfortunately, the only way to use istream_iterator is to read the whole line, including the standby load, into one vector
        std::vector<int> mixed_cycle_data{std::istream_iterator<int>{power_data}, std::istream_iterator<int>{}};
        //Take the standby load value off the end of the vector, leaving just the cycle data
        cur_device.standby_load = FixedPoint_t{static_cast<long>(mixed_cycle_data.back())};
        mixed_cycle_data.pop_back();
        //Cycle data alternates between load per cycle and minutes per cycle
        cur_device.load_per_cycle.resize(mixed_cycle_data.size()/2);
        cur_device.time_per_cycle.resize(cur_device.load_per_cycle.size());
        for(size_t i = 0; i < mixed_cycle_data.size(); i += 2) {
            cur_device.load_per_cycle[i/2] = FixedPoint_t(static_cast<long>(mixed_cycle_data[i]));
            cur_device.time_per_cycle[i/2] = mixed_cycle_data[i+1];
        }
    }

    while(std::getline(frequency_data_stream, line)) {
        std::istringstream frequency_data(line);
        //All text until the first tab is the name, then there are two tab-delimited doubles
        string name;
        std::getline(frequency_data, name, '\t');
        frequency_data >> possible_devices[name].weekday_frequency;
        frequency_data >> possible_devices[name].weekend_frequency;
    }

    while(std::getline(hourly_probability_stream, line)) {
        std::istringstream hourly_prob_data(line);
        string name;
        std::getline(hourly_prob_data, name, '\t');
        Device& cur_device = possible_devices[name];
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

inline bool device_already_picked(const std::list<Device>& existing_devices, const std::regex& name_pattern) {
    for(const auto& existing_device : existing_devices) {
        if(std::regex_search(existing_device.name, name_pattern))
            return true;
    }
    return false;
}

void Simulator::setup_simulation(int num_homes, const string& device_power_data_file, const string& device_frequency_data_file,
        const string& device_probability_data_file, const string& device_saturation_data_file) {
    read_devices_from_files(device_power_data_file, device_frequency_data_file, device_probability_data_file, device_saturation_data_file);
    modulus = util::get_valid_prime_modulus(num_homes);
    ProtocolState_t::init_failures_tolerated(modulus);
    //Initialize the SimCrypto instance
    sim_crypto = std::make_unique<SimCrypto>(modulus);
    //Initialize the utility
    utility_client = std::make_unique<UtilityClient>(modulus, utility_network_client_builder(sim_network),
            crypto_library_builder_utility(*sim_crypto), timer_manager_builder_utility(event_manager));
    PriceFunction sim_energy_price = [](const int time_of_day) {
        if(time_of_day > 17 && time_of_day < 20) {
            return Money(0.0734);
        } else {
            return Money(0.0612);
        }
    };
    while((int) meter_clients.size() < num_homes) {
        std::discrete_distribution<> income_distribution({25, 50, 25});
        int income_choice = income_distribution(random_engine);
        IncomeLevel income_level = income_choice == 0 ? IncomeLevel::POOR :
                (income_choice == 1 ? IncomeLevel::AVERAGE : IncomeLevel::RICH);
        //Pick what devices this home owns based on their saturation percentages
        std::list<Device> home_devices;
        for(const auto& device_saturation : devices_saturation) {
            //Devices that end in digits have multiple "versions," and only one of them should be in home_devices
            std::regex ends_in_digits(".*[0-9]$", std::regex::extended);
            if(std::regex_match(device_saturation.first, ends_in_digits)) {
                std::regex name_prefix(device_saturation.first.substr(0, device_saturation.first.length()-2) + string(".*"), std::regex::extended);
                if(device_already_picked(home_devices, name_prefix)) {
                    continue;
                }

            }
            //Homes have either a window or central AC but not both
            std::regex conditioner("conditioner");
            if(std::regex_search(device_saturation.first, conditioner) && device_already_picked(home_devices, conditioner)) {
                continue;
            }
            //Otherwise, randomly decide whether to include this device, based on its saturation
            double saturation_as_fraction = device_saturation.second / 100.0;
            if(std::binomial_distribution<>(saturation_as_fraction)(random_engine)) {
                home_devices.emplace_back(possible_devices[device_saturation.first]);
            }
        }
        //First construct a meter and keep a pointer to it in meters, then
        //construct a MeterClient for that meter (by emplacing it in the vector)
        int next_id = meter_clients.size();
        std::shared_ptr<Meter> new_meter = std::make_shared<Meter>(income_level, home_devices, sim_energy_price);
        meters.push_back(new_meter);
        meter_clients.emplace_back(std::make_unique<MeterClient>(next_id, modulus, new_meter, network_client_builder(sim_network),
                crypto_library_builder(*sim_crypto), timer_manager_builder(event_manager)));
    }
    //Assign secondary IDs to some meters
    int current_id = meter_clients.size();
    for(size_t double_id_pointer = 0; double_id_pointer < modulus - meter_clients.size(); ++double_id_pointer) {
        meter_clients[double_id_pointer]->set_second_id(current_id);
        virtual_meter_clients.emplace(current_id, std::ref(*meter_clients[double_id_pointer]));
        //Ugh, if it wasn't for needing to do this, I wouldn't have to expose MeterClient's NetworkClient.
        sim_network->connect_meter(meter_clients[double_id_pointer]->get_network_client(), current_id);
        current_id++;
    }

    sim_network->finish_setup();
    sim_crypto->finish_setup();

}

void Simulator::setup_queries(const std::set<QueryMode>& query_options) {
    using namespace messaging;
    if(query_options.find(QueryMode::ONLY_ONE_QUERY) != query_options.end()) {
        for(int timestep = 0; timestep < TOTAL_TIMESTEPS; ++timestep) {
            for(const auto& meter : meters) {
                event_manager.submit([meter](){ meter->simulate_usage_timestep();}, timesteps::millisecond(timestep), "Simulate electricity usage timestep");
            }
            if(timestep > 0 && timesteps::minute(timestep) % 60 == 0) {
                auto test_query = std::make_shared<QueryRequest>(QueryType::AVAILABLE_OFFSET_BREAKDOWN, 60, 0);
                event_manager.submit([test_query, this](){ utility_client->start_query(test_query); },
                        timesteps::millisecond(timestep) + 1, "Start query from utility");
            }
        }
    } else {
        int query_number = 0;
        for(int timestep = 0; timestep < TOTAL_TIMESTEPS; ++timestep) {
            for(const auto& meter : meters) {
                event_manager.submit([meter](){ meter->simulate_usage_timestep();}, timesteps::millisecond(timestep), "Simulate electricity usage timestep");
            }
            long query_start_time = timesteps::millisecond(timestep) + 1;
            if(timestep > 0 && timesteps::minute(timestep) % 60 == 0) {
                std::list<std::shared_ptr<QueryRequest>> queries;
                if(query_options.count(QueryMode::HOUR_QUERIES) > 0) {
                    hour_query_numbers[query_number] = query_start_time;
                    queries.emplace_back(std::make_shared<QueryRequest>(QueryType::AVAILABLE_OFFSET_BREAKDOWN, 60, query_number));
                }
                if(query_options.count(QueryMode::HALF_HOUR_QUERIES) > 0) {
                    int next_query_num = query_number + query_options.count(QueryMode::HOUR_QUERIES); //will be 1 if the option was set, or 0 if it was not
                    half_hour_query_numbers[next_query_num] = query_start_time;
                    queries.emplace_back(std::make_shared<QueryRequest>(QueryType::AVAILABLE_OFFSET_BREAKDOWN, 30, next_query_num));
                }
                if(query_options.count(QueryMode::QUARTER_HOUR_QUERIES) > 0) {
                    int next_query_num = query_number + query_options.count(QueryMode::HOUR_QUERIES) + query_options.count(QueryMode::HALF_HOUR_QUERIES);
                    quarter_hour_query_numbers[next_query_num] = query_start_time;
                    queries.emplace_back(std::make_shared<QueryRequest>(QueryType::AVAILABLE_OFFSET_BREAKDOWN, 15, next_query_num));
                }
                event_manager.submit([queries, this](){ utility_client->start_queries(queries); }, query_start_time, "Start query batch at utility");
                query_number += queries.size();
            } else if(timestep > 0 && timesteps::minute(timestep) % 30 == 0) {
                std::list<std::shared_ptr<QueryRequest>> queries;
                if(query_options.count(QueryMode::HALF_HOUR_QUERIES) > 0) {
                    half_hour_query_numbers[query_number] = query_start_time;
                    queries.emplace_back(std::make_shared<QueryRequest>(QueryType::AVAILABLE_OFFSET_BREAKDOWN, 30, query_number));
                }
                if(query_options.count(QueryMode::QUARTER_HOUR_QUERIES) > 0) {
                    int next_query_num = query_number + query_options.count(QueryMode::HALF_HOUR_QUERIES);
                    quarter_hour_query_numbers[next_query_num] = query_start_time;
                    queries.emplace_back(std::make_shared<QueryRequest>(QueryType::AVAILABLE_OFFSET_BREAKDOWN, 15, next_query_num));
                }
                event_manager.submit([queries, this](){ utility_client->start_queries(queries); }, query_start_time, "Start query batch at utility");
                query_number += queries.size();
            } else if(timestep > 0 && timesteps::minute(timestep) % 15 == 0) {
                if(query_options.count(QueryMode::QUARTER_HOUR_QUERIES) > 0) {
                    quarter_hour_query_numbers[query_number] = query_start_time;
                    auto quarter_hour_query = std::make_shared<QueryRequest>(QueryType::AVAILABLE_OFFSET_BREAKDOWN, 15, query_number);
                    event_manager.submit([quarter_hour_query, this](){ utility_client->start_query(quarter_hour_query); }, query_start_time, "Start quarter-hour query at utility");
                    query_number++;
                }
            }
        }
    }
}

void Simulator::run(const std::set<QueryMode>& query_options) {
    setup_queries(query_options);
    event_manager.run_simulation();
    //TODO: write output
}


} /* namespace simulation */
} /* namespace pddm */
