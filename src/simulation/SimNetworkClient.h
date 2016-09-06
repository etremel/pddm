/*
 * SimNetwork.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <vector>
#include <queue>

#include "../NetworkClient.h"
#include "../messaging/MessageType.h"
#include "EventManager.h"
#include "Network.h"

namespace pddm {
//Forward declaration, because MeterClient is defined in terms of NetworkClient
class MeterClient;

namespace simulation {

class Network;

/**
 * A network client for meters in the simulation. Buffers messages locally until
 * the simulated meter is ready to receive them (i.e. done processing the current
 * message, in simulated processing time).
 */
class SimNetworkClient : public NetworkClient {
        /** Pair associating an untyped (void*) message pointer and its (enum value) actual type. */
        using TypeMessagePair = std::pair<messaging::MessageType, std::shared_ptr<void>>;
    private:
        MeterClient& meter_client;
        std::shared_ptr<Network> network;
        EventManager& event_manager;
        /** Microseconds the client has been delayed, if it's been delayed less than 1ms.
         * Once this reaches 1ms, the client becomes busy and this count is reset. */
        long long accumulated_delay_micros;
        /** True if the client has been delayed (i.e. by cryptography) and the simulation
         * time has not yet reached busy_until_time. */
        bool client_is_busy;
        /** Simulation time, in milliseconds, at which the client will be done
         * with its current processing load (i.e. cryptography). */
        long long busy_until_time;
        /** Reference to the Event that will wake up the client at busy_until_time. */
        std::weak_ptr<Event> busy_done_event;
        /** Mixed-type list of incoming messages. */
        std::queue<TypeMessagePair> incoming_message_queue;

        class SendFunctor;

        void send(std::shared_ptr<std::list<TypeMessagePair>> untyped_messages, const int recipient_id);

        void resume_from_busy();

    public:
        SimNetworkClient(MeterClient& owning_meter_client, const std::shared_ptr<Network>& network) :
            meter_client(owning_meter_client), network(network), event_manager(network->event_manager),
            accumulated_delay_micros(0), client_is_busy(false), busy_until_time(0) {};
        virtual ~SimNetworkClient() = default;
        //Inherited from NetworkClient
        void send(const std::list<std::shared_ptr<messaging::OverlayTransportMessage>>& messages, const int recipient_id);
        void send(const std::shared_ptr<messaging::AggregationMessage>& message, const int recipient_id);
        void send(const std::shared_ptr<messaging::PingMessage>& message, const int recipient_id);
        void send(const std::shared_ptr<messaging::SignatureRequest>& message);

        /** Called by the simulated Network when the meter should receive a message. */
        void receive_message(const messaging::MessageType& message_type, const std::shared_ptr<void>& message);
        /** Instructs the client to consider its MeterClient "busy" for a period of time.
         * Called by the SimCryptoLibrary when the client does a crypto operation. */
        void delay_client(const int delay_time_micros);

};


//How can I get this to work??
//template<messaging::MessageType MType, typename Ret>
//std::shared_ptr<Ret> cast_helper(const std::shared_ptr<void>& message) {
//    using namespace messaging;
//    switch(MType) {
//    case MessageType::OVERLAY:
//        return std::static_pointer_cast<OverlayTransportMessage>(message);
//    case MessageType::AGGREGATION:
//        return std::static_pointer_cast<AggregationMessage>(message);
//    case MessageType::PING:
//        return std::static_pointer_cast<PingMessage>(message);
//    case MessageType::QUERY_REQUEST:
//        return std::static_pointer_cast<QueryRequest>(message);
//    case MessageType::SIGNATURE_REQUEST:
//        return std::static_pointer_cast<SignatureRequest>(message);
//    case MessageType::SIGNATURE_RESPONSE:
//        return std::static_pointer_cast<SignatureResponse>(message);
//    default:
//        return message;
//    }
//}

std::function<SimNetworkClient (MeterClient&)> network_client_builder(const std::shared_ptr<Network>& network);
} /* namespace simulation */
} /* namespace psm */
