/*
 * SimUtilityNetworkClient.h
 *
 *  Created on: May 31, 2016
 *      Author: edward
 */

#pragma once

#include <functional>
#include <memory>
#include <utility>

#include "../messaging/MessageType.h"
#include "../UtilityNetworkClient.h"

namespace pddm {
class UtilityClient;
}

namespace pddm {
namespace simulation {

class Network;

class SimUtilityNetworkClient: public UtilityNetworkClient {
        /** Pair associating an untyped (void*) message pointer and its (enum value) actual type. */
        using TypeMessagePair = std::pair<messaging::MessageType, std::shared_ptr<void>>;
    private:
        UtilityClient& utility_client;
        std::shared_ptr<Network> network;

        void send(const TypeMessagePair& untyped_message, const int meter_id);

    public:
        SimUtilityNetworkClient(UtilityClient& owning_utility_client, const std::shared_ptr<Network>& network) :
            utility_client(owning_utility_client), network(network) {}
        virtual ~SimUtilityNetworkClient() = default;
        //Inherited from UtilityNetworkClient
        void send(const std::shared_ptr<messaging::QueryRequest>& message, const int recipient_id);
        void send(const std::shared_ptr<messaging::SignatureResponse>& message, const int recipient_id);

        /** Called by the simulated Network when the client should receive a message. */
        void receive_message(const messaging::MessageType& message_type, const std::shared_ptr<void>& message);

};

std::function<SimUtilityNetworkClient (UtilityClient&)> utility_network_client_builder(const std::shared_ptr<Network>& network);


} /* namespace simulation */
} /* namespace pddm */

