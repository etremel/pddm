#pragma once

#include <cmath>
#include <memory>
#include <vector>
#include <list>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include "ProtocolState.h"
#include "Configuration.h"
#include "ConfigurationIncludes.h"
#include "FixedPoint_t.h"
#include "messaging/AggregationMessage.h"
#include "messaging/OverlayMessage.h"
#include "messaging/OverlayTransportMessage.h"
#include "messaging/PathOverlayMessage.h"
#include "messaging/ValueTuple.h"
#include "messaging/ValueContribution.h"
#include "messaging/OnionBuilder.h"
#include "util/PathFinder.h"
#include "util/Overlay.h"
#include "TreeAggregationState.h"
#include "util/OStreams.h"

#ifdef SIM_NETWORK
#include "simulation/DebugState.h"
#endif

#ifdef SIM_NETWORK
#define SIM_DEBUG(expr) do { expr } while (0)
#else
#define SIM_DEBUG(expr) do { } while (0)
#endif

namespace pddm {

template<typename Impl>
ProtocolState<Impl>::ProtocolState(Impl* subclass_ptr, NetworkClient_t& network, CryptoLibrary_t& crypto,
        TimerManager_t& timer_library, const int num_meters, const int meter_id, const int num_aggregation_groups) :
        logger(spdlog::get("global_logger")), impl_this(subclass_ptr), network(network), crypto(crypto),
        timers(timer_library), meter_id(meter_id), num_meters(num_meters), log2n((int) std::ceil(std::log2(num_meters))),
        num_aggregation_groups(num_aggregation_groups), overlay_round(0), is_last_round(false),
        round_timeout_timer(-1), ping_response_from_predecessor(false) {
}

template<typename Impl>
void ProtocolState<Impl>::start_query(const std::shared_ptr<messaging::QueryRequest>& query_request, const std::vector<FixedPoint_t>& contributed_data) {
    overlay_round = -1;
    is_last_round = false;
    ping_response_from_predecessor = false;
    timers.cancel_timer(round_timeout_timer);
    proxy_values.clear();
    failed_meter_ids.clear();
    SIM_DEBUG(util::init_debug_state(););
    aggregation_phase_state = std::make_unique<TreeAggregationState>(meter_id, num_aggregation_groups, num_meters,
            network, query_request);
    std::vector<int> proxies = util::pick_proxies(meter_id, num_aggregation_groups, num_meters);
    logger->trace("Meter {} chose these proxies: {}", meter_id, proxies);
    my_contribution = std::make_shared<messaging::ValueTuple>(query_request->query_number, contributed_data, proxies);
    impl_this->start_query_impl(query_request, contributed_data);
}


/**
 * Helper method that generates an encrypted multicast of a ValueContribution
 * to the proxies it specifies, assuming the overlay is starting in round 0,
 * then ends the overlay round. This is common to both CtProtocolState and
 * BftProtocolState.
 * @param contribution The ValueContribution to multicast.
 */
template<typename Impl>
void ProtocolState<Impl>::encrypted_multicast_to_proxies(const std::shared_ptr<messaging::ValueContribution>& contribution) {
    //Find independent paths starting at round 0
    auto proxy_paths = util::find_paths(meter_id, contribution->value.proxies, num_meters, 0);
    logger->trace("Meter {} picked these proxy paths: {}", meter_id, proxy_paths);
    for(const auto& proxy_path : proxy_paths) {
        //Create an encrypted onion for this path and send it
        outgoing_messages.emplace_back(messaging::build_encrypted_onion(proxy_path,
                contribution,  contribution->value.query_num, crypto));
    }
    //Start the overlay by ending "round -1", which will send the messages at the start of round 0
    end_overlay_round();
}

template<typename Impl>
void ProtocolState<Impl>::start_aggregate_phase() {
    //Since we're now done with the overlay, stop the timeout waiting for the next round
    timers.cancel_timer(round_timeout_timer);
    //Initialize aggregation helper, assuming all value-arrays are the same size as the one this meter contributed
    aggregation_phase_state->initialize(my_contribution->value.size(), failed_meter_ids);
    //If this node is a leaf, aggregation might be done already
    impl_this->send_aggregate_if_done();
    //If not done already, check future messages for aggregation messages already received from children
    if(impl_this->is_in_aggregate_phase()) {
        for(auto message_iter = future_aggregation_messages.begin();
                message_iter != future_aggregation_messages.end(); ) {
            handle_aggregation_message(*message_iter);
            message_iter = future_aggregation_messages.erase(message_iter);
        }
    }
    //Set this because we're done with the overlay
    is_last_round = true;
}

/**
 * Logic for handling a timeout waiting for a message in the current round.
 * If the predecessor node has responded to a ping recently, we ping it
 * again and keep waiting. If not, we give up and move to the next round.
 */
template<typename Impl>
void ProtocolState<Impl>::handle_round_timeout() {
    if(ping_response_from_predecessor) {
        ping_response_from_predecessor = false;
        const int predecessor = util::gossip_predecessor(meter_id, overlay_round, num_meters);
        logger->trace("Meter {} continuing to wait for round {}, got a response from {} recently", meter_id, overlay_round, predecessor);
        round_timeout_timer = timers.register_timer(OVERLAY_ROUND_TIMEOUT, [this](){handle_round_timeout();});
        auto ping = std::make_shared<messaging::PingMessage>(meter_id, false);
        auto success = network.send(ping, predecessor);
        if(!success) {
            logger->debug("Meter {} detected that meter {} just went down after responding to a ping", meter_id, predecessor);
            failed_meter_ids.emplace(predecessor);
        }
    } else {
        logger->debug("Meter {} timed out waiting for an overlay message for round {}", meter_id, overlay_round);
        end_overlay_round();
    }
}

template<typename Impl>
void ProtocolState<Impl>::super_end_overlay_round() {
    timers.cancel_timer(round_timeout_timer);
    SIM_DEBUG(logger->trace("Meter {} ending round {} at time {}", meter_id, overlay_round, util::print_time()););
    //If the last round is ending, the only thing we need to do is cancel the timeout
    if(is_last_round)
        return;

    overlay_round++;
    ping_response_from_predecessor = false;
    //Send outgoing messages at the start of the next round
    send_overlay_message_batch();

    round_timeout_timer = timers.register_timer(OVERLAY_ROUND_TIMEOUT, [this](){handle_round_timeout();});

    const int predecessor = util::gossip_predecessor(meter_id, overlay_round, num_meters);
    if(failed_meter_ids.find(predecessor) == failed_meter_ids.end()) {
        //Send a ping to the predecessor meter to see if it's still alive
        auto ping = std::make_shared<messaging::PingMessage>(meter_id, false);
        //This turns out to be really important: Checking whether this ping succeeds
        //is the most common way of detecting that a node has failed
        auto success = network.send(ping, predecessor);
        if(!success) {
            logger->debug("Meter {} detected that meter {} is down", meter_id, predecessor);
            failed_meter_ids.emplace(predecessor);
        }
    }

    //Check future messages in case messages for the next round have already been received
    ptr_list<messaging::OverlayTransportMessage> received_messages;
    for(auto message_iter = future_overlay_messages.begin();
            message_iter != future_overlay_messages.end(); ) {
        if((*message_iter)->sender_round == overlay_round
                && std::static_pointer_cast<messaging::OverlayTransportMessage::body_type>(
                        (*message_iter)->body)->query_num ==
                        get_current_query_num()) {
            received_messages.emplace_back(*message_iter);
            message_iter = future_overlay_messages.erase(message_iter);
        } else {
            ++message_iter;
        }
    }

    //Cache the last known value of overlay_round, because end_overlay_round()
    //might be called from inside one of these handle()s, so we might already
    //be another round ahead when they return
    const int local_overlay_round = overlay_round;
    for(const auto& message : received_messages) {
        handle_overlay_message(message);
    }
    //If end_overlay_round() hasn't already been called for another reason,
    //and the predecessor is known to be dead, immediately end the current round
    if(local_overlay_round == overlay_round && failed_meter_ids.find(predecessor) != failed_meter_ids.end()) {
        logger->trace("Meter {} ending round early, predecessor {} is dead", meter_id, predecessor);
        end_overlay_round();
    }

}

template<typename Impl>
void ProtocolState<Impl>::send_overlay_message_batch() {
    const int comm_target = util::gossip_target(meter_id, overlay_round, num_meters);
    ptr_list<messaging::OverlayTransportMessage> messages_to_send;
    //First, check waiting messages to see if some are now in the right round
    for(auto message_iter = waiting_messages.begin();
            message_iter != waiting_messages.end(); ) {
        if((*message_iter)->destination == comm_target) {
            //wrap it up in a new OverlayTransportMessage, then delete from waiting_messages
            messages_to_send.emplace_back(std::make_shared<messaging::OverlayTransportMessage>(
                    meter_id, overlay_round, false, *message_iter));
            message_iter = waiting_messages.erase(message_iter);
        } else {
            ++message_iter;
        }
    }
    //Next, check messages generated by the protocol this round to see if they should be sent or held
    for(const auto& overlay_message : outgoing_messages) {
        if(overlay_message->flood || overlay_message->destination == comm_target) {
            messages_to_send.emplace_back(std::make_shared<messaging::OverlayTransportMessage>(
                    meter_id, overlay_round, false, overlay_message));
        } else {
            waiting_messages.emplace_back(overlay_message);
        }
    }
//    logger->trace("Meter {} starting round {}. Size of messages_to_send: {}; size of waiting_messages: {}", meter_id, overlay_round, messages_to_send.size(), waiting_messages.size());
    outgoing_messages.clear();
    //Now, send all the messages, marking the last one as final
    if(!messages_to_send.empty()) {
        messages_to_send.back()->is_final_message = true;
        auto success = network.send(messages_to_send, comm_target);
        if(!success) {
            logger->debug("Meter {} detected that meter {} is down", meter_id, comm_target);
            failed_meter_ids.emplace(comm_target);
        }
    } else {
        //If we didn't send anything this round, send an empty message to ensure the target can advance his round
        auto dummy_message = std::make_shared<messaging::OverlayMessage>(
                get_current_query_num(), comm_target, nullptr);
        auto dummy_transport = std::make_shared<messaging::OverlayTransportMessage>(
                meter_id, overlay_round, true, dummy_message);
        logger->trace("Meter {} sending a dummy message to meter {}", meter_id, comm_target);
        auto success = network.send({dummy_transport}, comm_target);
        if(!success) {
            logger->debug("Meter {} detected that meter {} is down", meter_id, comm_target);
            failed_meter_ids.emplace(comm_target);
        }
    }
}

/**
 * Processes an overlay message that has been received for the current
 * round. The superclass implementation only resets the message timeout for
 * this round and appends the message to waiting_messages if it needs to be
 * forwarded. Subclasses should add phase-specific handling for the message
 * and end the round if it is the final message
 * @param message An overlay message that should be handled by this meter
 *        in the current round
 */
template<typename Impl>
void ProtocolState<Impl>::handle_overlay_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message) {
    if(impl_this->is_in_overlay_phase()) {
        timers.cancel_timer(round_timeout_timer);
        round_timeout_timer = timers.register_timer(OVERLAY_ROUND_TIMEOUT, [this](){handle_round_timeout();});
    }
    //The only valid MessageBody for an OverlayTransportMessage is an OverlayMessage
    auto wrapped_message = std::static_pointer_cast<messaging::OverlayMessage>(message->body);
    if(wrapped_message->is_encrypted) {
        //Replace the pointer in the OTM with the decrypted body, throwing away the encrypted data,
        //since this makes it easier to pass the decrypted message to the subclass handler method
        message->body = crypto.rsa_decrypt(wrapped_message);
    }
    if(auto path_overlay_message = std::dynamic_pointer_cast<messaging::PathOverlayMessage>(message->body)) {
        if(!path_overlay_message->remaining_path.empty()) {
            //Pop remaining_path into destination and add to waiting_messages
            path_overlay_message->destination = path_overlay_message->remaining_path.front();
//            path_overlay_message->remaining_path.erase(path_overlay_message->remaining_path.begin());
            path_overlay_message->remaining_path.pop_front();
            waiting_messages.emplace_back(path_overlay_message);
        }
    }
    impl_this->handle_overlay_message_impl(message);
}

/**
 * Processes a ping message from another meter, for the purpose of
 * detecting failures. Either responds if it is a request, or locally
 * records the fact that the predecessor responded to a ping recently.
 * @param message The ping message.
 */
template<typename Impl>
void ProtocolState<Impl>::handle_ping_message(const std::shared_ptr<messaging::PingMessage>& message) {
    if(!message->is_response) {
        //If this is a ping request, send a response back
        auto reply = std::make_shared<messaging::PingMessage>(meter_id, true);
        logger->trace("Meter {} replying to a ping from {}", meter_id, message->sender_id);
        network.send(reply, message->sender_id);
    } else if (message->sender_id == util::gossip_predecessor(meter_id, overlay_round, num_meters)) {
        //If this is a ping response and we still care about it
        //(the sender is our predecessor), take note
        ping_response_from_predecessor = true;
    }
}

template<typename Impl>
void ProtocolState<Impl>::buffer_future_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message) {
    future_overlay_messages.push_back(message);
}

template<typename Impl>
void ProtocolState<Impl>::buffer_future_message(const std::shared_ptr<messaging::AggregationMessage>& message) {
    future_aggregation_messages.push_back(message);
}

template<typename Impl>
void ProtocolState<Impl>::handle_aggregation_message(const std::shared_ptr<messaging::AggregationMessage>& message) {
    aggregation_phase_state->handle_message(*message);
    impl_this->send_aggregate_if_done();
}


}
