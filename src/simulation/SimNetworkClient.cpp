/*
 * SimNetwork.cpp
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#include <functional>
#include <memory>
#include <list>
#include <cassert>
#include "SimNetworkClient.h"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include "../MeterClient.h"
#include "Network.h"
#include "../messaging/QueryRequest.h"
#include "EventManager.h"

namespace pddm {
namespace simulation {

using std::static_pointer_cast;
using std::list;
using std::shared_ptr;
using std::unique_ptr;
using std::make_unique;
using std::make_shared;


/**
 * Helper method that bridges between the many typed "send" functions and the
 * simulated Network's untyped interface. Taking the message list as a shared_ptr,
 * which is copied into the send event's lambda, ensures that the list will stay
 * around until the event fires without copying the list. Note that the pointer
 * isn't really "shared" because this function does not keep any copies of it
 * (its refcount will always be 1), but unique_ptr doesn't work with std::function.
 * @param untyped_messages A pointer to a list of (type, pointer-to-message) pairs
 * @param recipient_id
 */
bool SimNetworkClient::send(std::shared_ptr<std::list<TypeMessagePair>> untyped_messages, const int recipient_id) {
    //I don't actually need the return value of network->send since I can just ask is_failed()...
    //Of course, if it wasn't for the fact that this function needs to return immediately, while
    //the network->send might get wrapped in an event lambda, I wouldn't need is_failed()
    bool success = !network->is_failed(recipient_id);
    if(!client_is_busy) {
        network->send(*untyped_messages, meter_client.meter_id, recipient_id);
    } else {
        //Create an event that will send the messages at the end of the current delay
        //(if more delay is accumulated after this send, it shouldn't affect this send)
        event_manager.submit([untyped_messages, recipient_id, this]() {
            network->send(*untyped_messages, meter_client.meter_id, recipient_id);
        }, busy_until_time, "Send messages after client delay");
    }
    //In BFT mode, failed meters may not advertise the fact that they are failed, so we can't detect failures.
    if(std::is_same<ProtocolState_t, BftProtocolState>::value) {
        return true;
    } else {
        return success;
    }
};


/**
 * The first argument is the (enumerated) message type, which simulates the
 * sender sending a brief identifier before streaming the message bytes.
 * @param message_type The type of the message
 * @param message An untyped pointer to the message
 */
void SimNetworkClient::receive_message(const messaging::MessageType& message_type, const std::shared_ptr<void>& message) {
    if(!client_is_busy) {
        //We finished a whole number of milliseconds of delay and received the next message,
        //so discard any extra microseconds - they shouldn't affect the next send
        accumulated_delay_micros = 0;
        switch(message_type) {
        case messaging::OverlayTransportMessage::type:
            logger->trace("At time {}, meter {} received an overlay message: {}",
                    event_manager.get_current_time(),
                    meter_client.meter_id,
                    *static_pointer_cast<messaging::OverlayTransportMessage>(message));
            meter_client.handle_message(static_pointer_cast<messaging::OverlayTransportMessage>(message));
            break;
        case messaging::AggregationMessage::type:
            meter_client.handle_message(static_pointer_cast<messaging::AggregationMessage>(message));
            break;
        case messaging::PingMessage::type:
            meter_client.handle_message(static_pointer_cast<messaging::PingMessage>(message));
            break;
        case messaging::QueryRequest::type:
            meter_client.handle_message(static_pointer_cast<messaging::QueryRequest>(message));
            break;
        case messaging::SignatureResponse::type:
            meter_client.handle_message(static_pointer_cast<messaging::SignatureResponse>(message));
            break;
        default:
            logger->warn("Meter {} dropped a message it didn't know how to handle.", meter_client.meter_id);
            break;
        }
    } else {
        incoming_message_queue.emplace(message_type, message);
    }
}

bool SimNetworkClient::send(const std::list<std::shared_ptr<messaging::OverlayTransportMessage>>& messages, const int recipient_id) {
    num_messages_sent += messages.size();
    //This isn't really "shared," we give up the reference to it and the send event will have the only copy.
    shared_ptr<list<TypeMessagePair>> raw_message_list = make_shared<list<TypeMessagePair>>();
    for(auto& message : messages) {
        raw_message_list->emplace_back(messaging::MessageType::OVERLAY, static_pointer_cast<void>(message));
    }
    return send(std::move(raw_message_list), recipient_id);
}

bool SimNetworkClient::send(const std::shared_ptr<messaging::AggregationMessage>& message, const int recipient_id) {
    num_messages_sent++;
    shared_ptr<list<TypeMessagePair>> raw_message_list = make_shared<list<TypeMessagePair>>();
    raw_message_list->emplace_back(messaging::MessageType::AGGREGATION, static_pointer_cast<void>(message));
    return send(std::move(raw_message_list), recipient_id);
}

bool SimNetworkClient::send(const std::shared_ptr<messaging::PingMessage>& message, const int recipient_id) {
    //Ping messages don't count towards message count, since they're tiny compared to the other messages
    shared_ptr<list<TypeMessagePair>> raw_message_list = make_shared<list<TypeMessagePair>>();
    raw_message_list->emplace_back(messaging::MessageType::PING, static_pointer_cast<void>(message));
    return send(std::move(raw_message_list), recipient_id);
}

bool SimNetworkClient::send(const std::shared_ptr<messaging::SignatureRequest>& message) {
    num_messages_sent++;
    shared_ptr<list<TypeMessagePair>> raw_message_list = make_shared<list<TypeMessagePair>>();
    raw_message_list->emplace_back(messaging::MessageType::SIGNATURE_REQUEST, static_pointer_cast<void>(message));
    return send(std::move(raw_message_list), -1);
}

void SimNetworkClient::delay_client(const int delay_time_micros) {
    if(!client_is_busy) {
        accumulated_delay_micros += delay_time_micros;
        if(accumulated_delay_micros > 1000) {
            int delay_ms = accumulated_delay_micros / 1000;
            accumulated_delay_micros = accumulated_delay_micros % 1000;
            client_is_busy = true;
            busy_until_time = event_manager.get_current_time() + delay_ms;
            busy_done_event = event_manager.submit([this](){resume_from_busy();}, busy_until_time,
                    "Resume client " + std::to_string(meter_client.meter_id) + " after processing delay", true);
        }
    } else {
        accumulated_delay_micros += delay_time_micros;
        if(accumulated_delay_micros > 1000) {
            int delay_ms = accumulated_delay_micros / 1000;
            accumulated_delay_micros = accumulated_delay_micros % 1000;
            busy_until_time += delay_ms;
            //Cancel the existing wakeup timer and add a new, longer one
            if(!busy_done_event.expired())
                busy_done_event.lock()->cancel();
            busy_done_event = event_manager.submit([this](){resume_from_busy();}, busy_until_time,
                    "Resume client " + std::to_string(meter_client.meter_id) + " after processing delay", true);
        }
    }
}

void SimNetworkClient::resume_from_busy() {
    //Just in case we didn't cancel an old timeout in time
    if(event_manager.get_current_time() < busy_until_time) {
        return;
    }
    client_is_busy = false;
    //Receive each incoming message in order
    //If handling a message causes the client to become busy, stop processing -
    //another resume_from_busy has been created that will handle the next one
    while(!client_is_busy && !incoming_message_queue.empty()) {
        using namespace messaging;
        const MessageType& message_type = incoming_message_queue.front().first;
        const shared_ptr<void>& message = incoming_message_queue.front().second;
        switch(message_type) {
        case MessageType::OVERLAY:
            meter_client.handle_message(static_pointer_cast<OverlayTransportMessage>(message));
            break;
        case MessageType::AGGREGATION:
            meter_client.handle_message(static_pointer_cast<AggregationMessage>(message));
            break;
        case MessageType::PING:
            meter_client.handle_message(static_pointer_cast<PingMessage>(message));
            break;
        case MessageType::QUERY_REQUEST:
            meter_client.handle_message(static_pointer_cast<QueryRequest>(message));
            break;
        case MessageType::SIGNATURE_RESPONSE:
            meter_client.handle_message(static_pointer_cast<SignatureResponse>(message));
            break;
		default:
			logger->warn("Meter {} dropped a message it didn't know how to handle.", meter_client.meter_id);
			break;
        }
        incoming_message_queue.pop();
    }
}

std::function<SimNetworkClient (MeterClient&)> network_client_builder(const std::shared_ptr<Network>& network) {
    return [network](MeterClient& meter_client) {
      SimNetworkClient network_client(meter_client, network);
      network->connect_meter(network_client, meter_client.meter_id);
      return network_client;
    };
}
} /* namespace simulation */
} /* namespace psm */

