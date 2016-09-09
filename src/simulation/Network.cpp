/*
 * Network.cpp
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#include <stddef.h>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <cmath>

#include "Network.h"
#include "../messaging/MessageType.h"
#include "Event.h"
#include "SimNetworkClient.h"
#include "SimUtilityNetworkClient.h"

namespace pddm {
namespace simulation {

const int MIN_LATENCY = 2;

void Network::connect_meter(SimNetworkClient& meter_client, const int id) {
    meter_clients_setup.emplace(id, std::ref(meter_client));
    failed.resize(meter_clients_setup.size());
    failed[id] = false;
}

void Network::connect_utility(SimUtilityNetworkClient& utility) {
    this->utility = std::ref(utility);
}

/**
 * Copies the temporary map of meter client references to a vector. This only
 * works if there are no gaps in meter IDs, since vectors of reference_wrapper
 * can only be added to with push_back (not operator[]).
 */
void Network::finish_setup() {
    if(!meter_clients.empty())
        throw std::runtime_error("finish_setup called more than once!");
    for(int id = 0; id < meter_clients_setup.end()->first; ++id) {
        meter_clients.emplace_back(meter_clients_setup.at(id));
    }
    meter_clients_setup.clear();
}
/**
 * Each untyped pointer to a message in the list should be identified by its
 * message type (the first element of the pair). This simulates having the
 * network send a "message type identifier" before streaming the raw bytes of
 * the message.
 * @param messages A list of (message-type, pointer-to-message) pairs
 * @param recipient_id
 */
void Network::send(const std::list<std::pair<messaging::MessageType, std::shared_ptr<void>>>& messages, const int recipient_id) {
    //Handle messages sent to the utility
    if(recipient_id == -1) {
        event_manager.submit([this, messages](){
            for(const auto& message_pair : messages) {
                utility->get().receive_message(message_pair.first, message_pair.second);
            }
        }, event_manager.get_current_time() + generate_latency(),
        "Deliver messages to utility");
        return;
    }

    if(recipient_id < meter_clients.size()) {
        if(failed[recipient_id]) {
            //TODO: create some send error event for meters that failed
        } else {
            auto arrival_time = event_manager.get_current_time() + generate_latency();
            logger->trace("Sending {} messages to meter {} with latency {}, to arrive at {}", messages.size(), recipient_id, arrival_time-event_manager.get_current_time(), arrival_time);
            event_manager.submit([this, messages, recipient_id](){
                for(const auto& message_pair : messages) {
                    meter_clients[recipient_id].get().receive_message(message_pair.first, message_pair.second);
                }
            }, arrival_time, "Deliver messages to " + std::to_string(recipient_id));
        }
    } else {
        logger->warn("Attempted to send a message to meter with ID {}, but there is no such meter!", recipient_id);
//        throw std::runtime_error(std::string("No meter with id ") + std::to_string(recipient_id));
    }
}

void Network::mark_failed(const int meter_id) {
    failed[meter_id] = true;
}

void Network::reset_failures() {
    for(size_t i = 0; i < failed.size(); ++i) {
        failed[i] = false;
    }
}

int Network::generate_latency() {
    return (int) std::fmax(MIN_LATENCY + std::round(latency_distribution(latency_randomness)), 0);
}

} /* namespace simulation */
} /* namespace psm */

