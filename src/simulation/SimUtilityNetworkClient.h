/*
 * SimUtilityNetworkClient.h
 *
 *  Created on: May 31, 2016
 *      Author: edward
 */

#pragma once

#include "../UtilityNetworkClient.h"
#include "../UtilityClient.h"
#include "Network.h"
#include "../messaging/QueryRequest.h"
#include "../messaging/SignatureResponse.h"

namespace pddm {
namespace simulation {

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
        virtual ~SimUtilityNetworkClient();
        //Inherited from UtilityNetworkClient
        void send(const std::shared_ptr<messaging::QueryRequest>& message, const int recipient_id);
        void send(const std::shared_ptr<messaging::SignatureResponse>& message, const int recipient_id);

        /** Called by the simulated Network when the client should receive a message. */
        void receive_message(const messaging::MessageType& message_type, const std::shared_ptr<void>& message);

};

std::function<UtilityNetworkClient_t (UtilityClient&)> utility_network_client_builder(const std::shared_ptr<Network>& network);


} /* namespace simulation */
} /* namespace pddm */

