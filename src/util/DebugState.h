/**
 * @file DebugState.h
 * Global state and some associated functions used for debugging the simulation.
 * This should be removed from ProtocolState and its subclasses once the simulation
 * works, because it will break if MeterClient is compiled in non-simulated mode!
 * @date Sep 10, 2016
 * @author edward
 */

#pragma once

#include <spdlog/spdlog.h>
#include <memory>

#include "../simulation/SimParameters.h"
#include "../simulation/EventManager.h"

namespace pddm {

namespace util {

struct DebugState {
        //EventManager is a value-type wholly contained within Simulator, so we have to use a dangerous pointer to it here
        simulation::EventManager* event_manager;
        int num_finished_shuffle;
        int num_finished_echo;
        int num_finished_agreement;
        int num_finished_aggregate;
        int num_finished_scatter;
        int num_finished_gather;
};

DebugState& debug_state();

inline void init_debug_state() {
    debug_state().num_finished_shuffle = 0;
    debug_state().num_finished_echo = 0;
    debug_state().num_finished_agreement = 0;
    debug_state().num_finished_aggregate = 0;
    debug_state().num_finished_scatter = 0;
    debug_state().num_finished_gather = 0;
}

inline std::string print_time() {
    if(!debug_state().event_manager)
        return "null";
    return std::to_string(debug_state().event_manager->get_current_time());
}

inline void print_shuffle_status(const std::shared_ptr<spdlog::logger>& logger, const int num_meters) {
    if(debug_state().num_finished_shuffle == num_meters - simulation::METER_FAILURES_PER_QUERY) {
        logger->debug("All meters are finished with Shuffle");
    }
}

inline void print_scatter_status(const std::shared_ptr<spdlog::logger>& logger, const int num_meters) {
    if(debug_state().num_finished_scatter == num_meters - simulation::METER_FAILURES_PER_QUERY) {
        logger->info("All meters are finished with Scatter");
    }
}

inline void print_echo_status(const std::shared_ptr<spdlog::logger>& logger, const int meter_id, const int num_meters) {
    if(debug_state().num_finished_shuffle < num_meters - simulation::METER_FAILURES_PER_QUERY) {
        logger->warn("Meter {} finished with Echo, but {} meters are still in Shuffle phase!", meter_id, num_meters - debug_state().num_finished_shuffle - simulation::METER_FAILURES_PER_QUERY);
    }
    if(debug_state().num_finished_echo == num_meters - simulation::METER_FAILURES_PER_QUERY) {
        logger->debug("All meters are finished with Echo");
    }
}

inline void print_gather_status(const std::shared_ptr<spdlog::logger>& logger, const int meter_id, const int num_meters) {
    if(debug_state().num_finished_scatter < num_meters - simulation::METER_FAILURES_PER_QUERY) {
        logger->warn("Meter {} finished with Gather, but {} meters are still in Scatter phase!", meter_id, num_meters - debug_state().num_finished_shuffle - simulation::METER_FAILURES_PER_QUERY);
    }
    if(debug_state().num_finished_gather == num_meters - simulation::METER_FAILURES_PER_QUERY) {
        logger->info("All meters are finished with Gather");
    }
}

inline void print_agreement_status(const std::shared_ptr<spdlog::logger>& logger, const int meter_id, const int num_meters) {
    if(debug_state().num_finished_shuffle < num_meters - simulation::METER_FAILURES_PER_QUERY) {
        logger->warn("Meter {} finished with Agreement, but {} meters are still in Shuffle phase!", meter_id, num_meters - debug_state().num_finished_shuffle - simulation::METER_FAILURES_PER_QUERY);
    }
    if(debug_state().num_finished_agreement == num_meters - simulation::METER_FAILURES_PER_QUERY) {
        logger->debug("All meters are finished with Agreement");
    }
}

inline void print_aggregate_status(const std::shared_ptr<spdlog::logger>& logger, const int num_meters) {
    if(debug_state().num_finished_aggregate == num_meters - simulation::METER_FAILURES_PER_QUERY) {
        logger->debug("All meters are finished with Aggregate");
    }
}


}
}


