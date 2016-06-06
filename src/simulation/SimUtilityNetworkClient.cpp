/*
 * SimUtilityNetworkClient.cpp
 *
 *  Created on: May 31, 2016
 *      Author: edward
 */

#include <memory>
#include <list>

#include "SimUtilityNetworkClient.h"

using std::static_pointer_cast;
using std::make_pair;

namespace pddm {
namespace simulation {

void SimUtilityNetworkClient::send(const TypeMessagePair& untyped_message, const int meter_id) {
    std::list<TypeMessagePair> trivial_list {untyped_message};
    network->send(trivial_list, meter_id);
}

void SimUtilityNetworkClient::send(const std::shared_ptr<messaging::QueryRequest>& message, const int recipient_id) {
    send(make_pair(messaging::MessageType::QUERY_REQUEST, message), recipient_id);
}

void SimUtilityNetworkClient::send(const std::shared_ptr<messaging::SignatureResponse>& message, const int recipient_id) {
    send(make_pair(messaging::MessageType::SIGNATURE_RESPONSE, message), recipient_id);
}

void SimUtilityNetworkClient::receive_message(const messaging::MessageType& message_type, const std::shared_ptr<void>& message) {
    using namespace messaging;
    switch(message_type) {
    case MessageType::AGGREGATION:
        utility_client.handle_message(static_pointer_cast<AggregationMessage>(message));
        break;
    case MessageType::SIGNATURE_REQUEST:
        utility_client.handle_message(static_pointer_cast<SignatureRequest>(message));
        break;
    }
}


std::function<UtilityNetworkClient_t(UtilityClient&)> utility_network_client_builder(const std::shared_ptr<Network>& network) {
    return [network](UtilityClient& utility_client) {
        SimUtilityNetworkClient network_client(utility_client, network);
        network->connect_utility(network_client);
        return network_client;
    };
}

} /* namespace simulation */
} /* namespace pddm */
