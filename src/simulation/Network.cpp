/*
 * Network.cpp
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#include <algorithm>
#include <stdexcept>
#include <string>
#include <cmath>
#include <functional>

#include "Network.h"

namespace pddm {
namespace simulation {

const int MIN_LATENCY = 2;

void Network::connect_meter(const SimNetworkClient& meter_client, const int id) {
    meter_clients.resize(std::min(static_cast<int>(meter_clients.size()), id));
    meter_clients[id] = std::ref(meter_client);
    failed.resize(meter_clients.size());
    failed[id] = false;
}

void Network::connect_utility(const SimUtilityNetworkClient& utility) {
    this->utility = std::ref(utility);
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
        event_manager.submit([messages](){
            for(const auto& message_pair : messages) {
                utility.get().receive_message(message_pair.first, message_pair.second);
            }
        }, event_manager.simulation_time + generate_latency());
        return;
    }

    if(recipient_id - 1 < meter_clients.size() && meter_clients[recipient_id] != nullptr) {
        if(failed[recipient_id]) {
            //TODO: create some send error event for meters that failed
        } else {
            event_manager.submit([messages, recipient_id](){
                for(const auto& message_pair : messages) {
                    meter_clients[recipient_id].get().receive_message(message_pair.first, message_pair.second);
                }
            }, event_manager.simulation_time + generate_latency());
        }
    } else {
        throw std::runtime_error(std::string("No meter with id ") + std::to_string(recipient_id));
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
    return (int) fmax(MIN_LATENCY + round(latency_distribution(latency_randomness)), 0);
}

} /* namespace simulation */
} /* namespace psm */

