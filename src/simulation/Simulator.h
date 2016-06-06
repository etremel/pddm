/*
 * Simulator.h
 *
 *  Created on: May 27, 2016
 *      Author: edward
 */

#pragma once

#include <vector>
#include <random>
#include <functional>
#include <memory>

#include "../MeterClient.h"
#include "Device.h"

namespace pddm {
namespace simulation {

enum class QueryMode { HOUR_QUERIES, HALF_HOUR_QUERIES, QUARTER_HOUR_QUERIES, ONLY_ONE_QUERY};

class Simulator {
    private:
        EventManager event_manager;
        std::shared_ptr<Network> sim_network;
        int modulus;
        /** All of the meter clients in the simulation; this is where they are stored. */
        std::vector<MeterClient> meter_clients;
        /** Index of meter clients that have secondary IDs.
         * Maps the secondary ID to a reference to an object in meter_clients. */
        std::map<int, std::reference_wrapper<MeterClient>> virtual_meter_clients;
        /** References to the simulated Meters owned by the MeterClients,
         * kept here so that the simulation can make them generate measurements */
        std::vector<std::reference_wrapper<Meter>> meters;
        //This should be a value type, but it can't be initialized until we know the number of meters, so it must be on the heap
        std::unique_ptr<UtilityClient> utility_client;
        //Same with this
        std::unique_ptr<SimCrypto> sim_crypto;
        SimTimerManager sim_timers;

        /** List of all possible Device objects, indexed by their name. */
        std::map<std::string, Device> possible_devices;
        /** Maps a device name to the saturation of that device, as a percentage. */
        std::map<std::string, double> devices_saturation;

        /** Source of randomness to feed assorted random distributions during setup. */
        std::mt19937 random_engine;

        /** Maps the query number for each hourly query to the simulation time at which it was issued */
        std::map<int, long> hour_query_numbers;
        /** Maps the query number for each half-hour query to the simulation time at which it was issued */
        std::map<int, long> half_hour_query_numbers;
        /** Maps the query number for each quarter-hour query to the simulation time at which it was issued */
        std::map<int, long> quarter_hour_query_numbers;

        void read_devices_from_files(const std::string& device_power_data_file,
                const std::string& device_frequency_data_file, const std::string& device_probability_data_file,
                const std::string& device_saturation_data_file);

    public:
        enum class QueryMode { HOUR_QUERIES, HALF_HOUR_QUERIES, QUARTER_HOUR_QUERIES, ONLY_ONE_QUERY };
        Simulator() : event_manager(), sim_network(std::make_shared<Network>(event_manager)), modulus(0), sim_timers(event_manager) {}
        void setup_simulation(int num_homes, const std::string& device_power_data_file,
                const std::string& device_frequency_data_file, const std::string& device_probability_data_file,
                const std::string& device_saturation_data_file);
        void setup_queries(const std::set<QueryMode>& query_options);
        void run(const std::set<QueryMode>& query_options);
};

} /* namespace simulation */
} /* namespace pddm */

