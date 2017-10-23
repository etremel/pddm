/**
 * @file Simulator.cpp
 *
 * @date May 27, 2016
 * @author edward
 */

#include "Simulator.h"

#include <cstdlib>
#include <cstddef>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <iomanip>
#include <regex>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include "../Configuration.h"
#include "../messaging/QueryRequest.h"
#include "../messaging/AggregationMessage.h"
#include "../util/Money.h"
#include "../util/Overlay.h"
#include "../util/Random.h"
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
#include "../util/ConfigParser.h"

using std::string;

namespace pddm {
namespace simulation {

void Simulator::setup_simulation(int num_homes, const string& device_power_data_file, const string& device_frequency_data_file,
        const string& device_probability_data_file, const string& device_saturation_data_file) {
    util::read_devices_from_files(device_power_data_file, device_frequency_data_file,
            device_probability_data_file, device_saturation_data_file,
            possible_devices, devices_saturation);

    util::debug_state().event_manager = &event_manager;

    modulus = util::get_valid_prime_modulus(num_homes);
    ProtocolState_t::init_failures_tolerated(modulus);
    //Initialize the SimCrypto instance
    sim_crypto = std::make_unique<SimCrypto>(modulus);
    //Initialize the utility
    utility_client = std::make_unique<UtilityClient>(modulus, utility_network_client_builder(sim_network),
            crypto_library_builder_utility(*sim_crypto), timer_manager_builder_utility(event_manager));
    using namespace std::placeholders;
    utility_client->register_query_callback(std::bind(&Simulator::query_finished_callback, this, _1, _2));
    PriceFunction sim_energy_price = [](const int time_of_day) {
        if(time_of_day > 17 && time_of_day < 20) {
            return Money(0.0734);
        } else {
            return Money(0.0612);
        }
    };
    while(meter_clients.size() < (std::size_t) num_homes) {
        std::discrete_distribution<> income_distribution({25, 50, 25});
        //First construct a meter and keep a pointer to it in meters, then
        //construct a MeterClient for that meter (by emplacing it in the vector)
        int next_id = meter_clients.size();
        std::shared_ptr<Meter> new_meter(generate_meter(income_distribution, possible_devices,
                devices_saturation, sim_energy_price, random_engine).release());
        meters.push_back(new_meter);
        meter_clients.emplace_back(std::make_unique<MeterClient>(next_id, modulus, new_meter, network_client_builder(sim_network),
                crypto_library_builder(*sim_crypto), timer_manager_builder(event_manager)));
    }
    //Assign secondary IDs to some meters
    int current_id = meter_clients.size();
    for(std::size_t double_id_pointer = 0; double_id_pointer < modulus - meter_clients.size(); ++double_id_pointer) {
        meter_clients[double_id_pointer]->set_second_id(current_id);
        virtual_meter_clients.emplace(current_id, std::ref(*meter_clients[double_id_pointer]));
        sim_network->connect_meter(meter_clients[double_id_pointer]->network_client, current_id);
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
            if(timesteps::minute(timestep) == 60) {
                long query_start_time = timesteps::millisecond(timestep) + 1;
                auto test_query = std::make_shared<QueryRequest>(QueryType::AVAILABLE_OFFSET_BREAKDOWN, 60, 0);
                hour_query_numbers[0] = query_start_time;
                event_manager.submit([test_query, this](){
                    fail_meters();
                    utility_client->start_query(test_query);
                }, query_start_time, "Start query from utility");
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

/**
 * It records the completion time of the query and, if necessary, resets simulated meter failures.
 * @param query_num The number of the query that completed.
 * @param result The result of the query.
 */
void Simulator::query_finished_callback(const int query_num, std::shared_ptr<messaging::AggregationMessageValue> result) {
    //query_num is a key in either hour_query_numbers, half_hour_query_numbers, or quarter_hour_query_numbers
    auto query_num_find = hour_query_numbers.find(query_num);
    if(query_num_find == hour_query_numbers.end()) {
        query_num_find = half_hour_query_numbers.find(query_num);
        if(query_num_find == half_hour_query_numbers.end()) {
            query_num_find = quarter_hour_query_numbers.find(query_num);
        }
    }
    long query_start_time = query_num_find->second;
    //Safer than push_back in case we ever get query results out of numeric order
    query_round_trip_times.resize(query_num + 1);
    query_round_trip_times[query_num] = event_manager.get_current_time() - query_start_time;

//    reset_meter_failures();
}

void Simulator::write_query_times(const std::string& file_timestamp) const {
    std::stringstream filename;
    if(std::is_same<ProtocolState_t, BftProtocolState>::value) {
        filename << "bft_";
    } else if (std::is_same<ProtocolState_t, HftProtocolState>::value) {
        filename << "hft_";
    } else if (std::is_same<ProtocolState_t, CtProtocolState>::value) {
        filename << "ct_";
    }
    if(METER_FAILURES_PER_QUERY == 0) {
        filename << "query_times_nofail";
    } else {
        filename << "query_times_failures";
    }
    filename << "_" << modulus << "_" << file_timestamp << ".csv";
    std::ofstream query_times_file(filename.str());
    if(query_round_trip_times.size() == 1) {
        query_times_file << modulus << "," << query_round_trip_times[0] << std::endl;
    } else {
        query_times_file << "QueryNum,Runtime" << std::endl;
        for(unsigned int query_num = 0; query_num <  query_round_trip_times.size(); ++query_num) {
            query_times_file << query_num << "," << query_round_trip_times[query_num] << std::endl;
        }
    }

}


std::tuple<int, int, int> to_hms(const long milliseconds) {
    auto qr = std::div(milliseconds, 1000l);
    qr = std::div(qr.quot, 60l);
    //sec = (ms / 1000) % 60
    auto s  = qr.rem;
    qr = std::div(qr.quot, 60l);
    //min = (ms / (1000*60)) % 60
    auto m  = qr.rem;
    auto h  = qr.quot;
    return std::make_tuple((int)h, (int)m, (int)s);
}

void Simulator::write_query_history(const std::string& file_timestamp) const {
    if(!hour_query_numbers.empty()) {
        std::stringstream filename;
        filename << "utility_60m_queries_" <<  modulus << "_" << file_timestamp << ".csv";
        std::ofstream hour_query_file(filename.str());
        for(const auto& query_time_pair : hour_query_numbers) {
            auto time_tuple = to_hms(query_time_pair.second);
            auto query_result = utility_client->get_query_result(query_time_pair.first);
            hour_query_file << std::setfill('0') << std::setw(2) << std::get<0>(time_tuple) << ":" <<
                    std::setw(2) << std::get<1>(time_tuple) << ":" <<
                    std::setw(2) << std::get<2>(time_tuple) <<
                    std::setfill(' ') << ", " << query_result << std::endl;
        }
    }
    if(!half_hour_query_numbers.empty()) {
        std::stringstream filename;
        filename << "utility_30m_queries_" <<  modulus << "_" << file_timestamp << ".csv";
        std::ofstream half_hour_query_file(filename.str());
        for(const auto& query_time_pair : half_hour_query_numbers) {
            auto time_tuple = to_hms(query_time_pair.second);
            auto query_result = utility_client->get_query_result(query_time_pair.first);
            half_hour_query_file << std::setfill('0') << std::setw(2) << std::get<0>(time_tuple) << ":" <<
                    std::setw(2) << std::get<1>(time_tuple) << ":" <<
                    std::setw(2) << std::get<2>(time_tuple) <<
                    std::setfill(' ') << ", " << query_result << std::endl;
        }
    }
    if(!quarter_hour_query_numbers.empty()) {
        std::stringstream filename;
        filename << "utility_15m_queries_" <<  modulus << "_" << file_timestamp << ".csv";
        std::ofstream quarter_hour_query_file(filename.str());
        for(const auto& query_time_pair : quarter_hour_query_numbers) {
            auto time_tuple = to_hms(query_time_pair.second);
            auto query_result = utility_client->get_query_result(query_time_pair.first);
            quarter_hour_query_file << std::setfill('0') << std::setw(2) << std::get<0>(time_tuple) << ":" <<
                    std::setw(2) << std::get<1>(time_tuple) << ":" <<
                    std::setw(2) << std::get<2>(time_tuple) <<
                    std::setfill(' ') << ", " << query_result << std::endl;
        }
    }
}

void Simulator::write_message_counts(const std::string& file_timestamp) const {
    std::stringstream filename;
    if(std::is_same<ProtocolState_t, BftProtocolState>::value) {
        filename << "bft_";
    } else if (std::is_same<ProtocolState_t, HftProtocolState>::value) {
        filename << "hft_";
    } else if (std::is_same<ProtocolState_t, CtProtocolState>::value) {
        filename << "ct_";
    }
    filename << "meter_messages_";
    if(METER_FAILURES_PER_QUERY == 0) {
        filename << "nofail";
    } else {
        filename << "failures";
    }
    filename << "_" << modulus << "_" << file_timestamp << ".csv";
    std::ofstream message_file(filename.str());
    for(unsigned int meter_id = 0; meter_id < meter_clients.size(); ++meter_id) {
        message_file << meter_id << "," << meter_clients[meter_id]->network_client.get_total_messages_sent() << std::endl;
    }
}

void Simulator::fail_meters() {
    if(METER_FAILURES_PER_QUERY < 1)
        return;
    auto failed_ids = util::pick_without_replacement(modulus, METER_FAILURES_PER_QUERY, random_engine);
//    if(METER_FAILURES_PER_QUERY == 7) {
//        failed_ids = {6, 22, 33, 63, 64, 68, 81};
//    }
    logger->info("Failing meters: {}", failed_ids);
    for(const auto& id : failed_ids) {
        sim_network->mark_failed(id);
    }
}

void Simulator::reset_meter_failures() {
    if(METER_FAILURES_PER_QUERY < 1)
        return;
    sim_network->reset_failures();
}

void Simulator::run(const std::set<QueryMode>& query_options) {
    setup_queries(query_options);
    event_manager.run_simulation();
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream file_timestamp;
    file_timestamp << std::put_time(std::localtime(&now), "%m%d-%H%M%S");
    if(WRITE_MESSAGE_STATS) {
        write_message_counts(file_timestamp.str());
    }
    if(WRITE_SIMULATION_RESULTS) {
        write_query_history(file_timestamp.str());
    }
    if(WRITE_QUERY_STATS) {
        write_query_times(file_timestamp.str());
    }
}


} /* namespace simulation */
} /* namespace pddm */
