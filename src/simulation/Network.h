/*
 * Network.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include <list>
#include <vector>
#include <memory>
#include <random>
#include <functional>

#include "EventManager.h"
#include "SimNetworkClient.h"
#include "../messaging/Message.h"

namespace pddm {
namespace simulation {

class Network {
    private:
        /** Contains a pointer to each meter's SimNetworkClient at the index
         * corresponding to its meter ID. */
        std::vector<std::reference_wrapper<SimNetworkClient>> meter_clients;
        /** Marks which meters are considered failed by the simulation;
         * they will never receive messages that are sent to them. */
        std::vector<bool> failed;
        /** The utility's SimNetworkClient; the utility's ID is -1.
         * Note that this is a reference, but it must be initialized after the
         * constructor is called, so it has to be wrapped. */
        std::reference_wrapper<SimUtilityNetworkClient> utility;
        EventManager& event_manager;
        std::mt19937_64 latency_randomness;
        std::normal_distribution latency_distribution;
        int generate_latency();

        //Let the simulated network clients reach in here to get the EventManager -
        //these two classes are tightly coupled anyway
        friend class SimNetworkClient;

    public:
        Network(EventManager& events_manager) : event_manager(events_manager), latency_distribution(4.0, 1.5) {}
        /** Adds a meter to the simulated network, registered to the given ID. */
        void connect_meter(const SimNetworkClient& meter_client, const int id);
        /** Adds the utility to the simulated network */
        void connect_utility(const SimUtilityNetworkClient& utility);
        /** Sends a stream of messages to a recipient identified by its ID. */
        void send(const std::list<std::pair<messaging::MessageType, std::shared_ptr<void>>>& messages, const int recipient_id);
        /** Marks a meter as "failed" for the duration of this simulation; it will not receive any messages. */
        void mark_failed(const int meter_id);
        /** Marks all meters as non-failed. */
        void reset_failures();
};

} /* namespace simulation */
} /* namespace psm */

