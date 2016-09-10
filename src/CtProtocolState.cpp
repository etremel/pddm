/*
 * CtProtocolState.cpp
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#include <algorithm>
#include <memory>
#include <spdlog/fmt/ostr.h>

#include "CtProtocolState.h"
#include "messaging/OverlayMessage.h"
#include "messaging/OverlayTransportMessage.h"
#include "messaging/QueryRequest.h"
#include "messaging/OnionBuilder.h"
#include "util/PathFinder.h"
#include "util/DebugState.h"

namespace pddm {

void CtProtocolState::start_query_impl(const std::shared_ptr<messaging::QueryRequest>& query_request, const std::vector<FixedPoint_t>& contributed_data) {
    //super.start_query()
    //Reinitialize aggregation state
    protocol_phase = CtProtocolPhase::SHUFFLE;
    echo_start_round = 0;

    //We don't actually need to sign contributions in crash-tolerant mode, but use ValueContribution anyway
    auto signed_contribution = std::make_shared<messaging::ValueContribution>(*my_contribution);

    encrypted_multicast_to_proxies(signed_contribution);
}

void CtProtocolState::handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message) {
    //super.handle_overlay_message()
    auto overlay_message = std::static_pointer_cast<messaging::OverlayMessage>(message->body);
    //Dummy messages will have a null payload
    if(overlay_message->body != nullptr) {
        /* If it's an encrypted onion that needs to be forwarded, the payload will be the next layer.
         * If the payload is not an OverlayMessage, it's either a PathOverlayMessage or the last layer
         * of the onion. The last layer of the onion will always have destination == meter_id (because
         * it was just received here), but a PathOverlayMessage that still needs to be forwarded will
         * have its destination already set to the next hop by the superclass handle_overlay_message.
         */
        if(auto enclosed_message = std::dynamic_pointer_cast<messaging::OverlayMessage>(overlay_message->body)){
            waiting_messages.emplace_back(enclosed_message);
        } else if(overlay_message->destination == meter_id){
            if(protocol_phase == CtProtocolPhase::SHUFFLE) {
                handle_shuffle_phase_message(*overlay_message);
            } else if(protocol_phase == CtProtocolPhase::ECHO) {
                handle_echo_phase_message(*overlay_message);
            }
        } //If destination didn't match, it was already added to waiting_messages
    }
    if(message->is_final_message && is_in_overlay_phase()) {
        end_overlay_round();
    }
}


void CtProtocolState::handle_shuffle_phase_message(const messaging::OverlayMessage& message) {
    //Drop messages that are received in the wrong phase (i.e. not ValueContributions) or have the wrong round number
    if(auto contribution = std::dynamic_pointer_cast<messaging::ValueContribution>(message.body)) {
        if(contribution->value.query_num == my_contribution->query_num) {
            logger->trace("Meter {} received proxy value: {}", meter_id, *contribution);
            proxy_values.emplace(contribution);
        } else {
            logger->warn("Meter {} rejected a proxy value because it had the wrong query number: {}", meter_id, *contribution);
        }
    } else if(message.body != nullptr) {
        logger->warn("Meter {} rejected a message because it was not a ValueContribution: {}", meter_id, message);
    }
}

void CtProtocolState::handle_echo_phase_message(const messaging::OverlayMessage& message) {
    if(auto contribution = std::dynamic_pointer_cast<messaging::ValueContribution>(message.body)) {
        if(contribution->value.query_num == my_contribution->query_num) {
            logger->trace("Meter {} received echoed proxy value in round {}: {}", meter_id, overlay_round, *contribution);
            proxy_values.emplace(contribution);
        } else {
            logger->warn("Meter {} rejected a proxy value: {}", meter_id, *contribution);

        }
    } else if(message.body != nullptr) {
        logger->warn("Meter {} rejected a message because it was not a ValueContribution: {}", meter_id, message);
    }
}

void CtProtocolState::send_aggregate_if_done() {
    if(aggregation_phase_state->done_receiving_from_children()) {
        aggregation_phase_state->compute_and_send_aggregate(proxy_values);
        protocol_phase = CtProtocolPhase::IDLE;
        logger->debug("Meter {} is finished with Aggregate", meter_id);
        util::debug_state().num_finished_aggregate++;
        util::print_aggregate_status(logger, num_meters);
    }
}

void CtProtocolState::end_overlay_round_impl() {
    //Determine if the Shuffle phase has ended
    if(protocol_phase == CtProtocolPhase::SHUFFLE
            && overlay_round >= FAILURES_TOLERATED + 2 * log2n + 1) {
        logger->debug("Meter {} is finished with Shuffle", meter_id);
        //Multicast each received value to its other proxies
        for(const auto& proxy_value : proxy_values) {
            //Remove this meter's ID from the list of proxies
            std::vector<int> other_proxies(proxy_value->value.proxies.size()-1);
            std::remove_copy(proxy_value->value.proxies.begin(),
                    proxy_value->value.proxies.end(), other_proxies.begin(), meter_id);
            auto proxy_paths = util::find_paths(meter_id, other_proxies, num_meters, overlay_round+1);
            logger->trace("Meter {} chose these paths for echo: {}", meter_id, proxy_paths);
            for(const auto& proxy_path : proxy_paths) {
                //Encrypt with the destination's public key, but don't make an onion
                outgoing_messages.emplace_back(crypto.rsa_encrypt(std::make_shared<messaging::PathOverlayMessage>(
                        get_current_query_num(), proxy_path, proxy_value), proxy_path.back()));
                //Does proxy_value need to be copied? I don't think so, it won't change
            }
        }
        echo_start_round = overlay_round;
        protocol_phase = CtProtocolPhase::ECHO;
        util::debug_state().num_finished_shuffle++;
        util::print_shuffle_status(logger, num_meters);
    }
    //Determine if the Echo phase has ended
    else if (protocol_phase == CtProtocolPhase::ECHO
            && overlay_round >= echo_start_round + FAILURES_TOLERATED + 2 * log2n + 1) {
        logger->debug("Meter {} is finished with Echo", meter_id);
        util::debug_state().num_finished_echo++;
        util::print_echo_status(logger, meter_id, num_meters);
        //Start the Aggregate phase
        protocol_phase = CtProtocolPhase::AGGREGATE;
        start_aggregate_phase();
    }
}

} /* namespace pddm */
