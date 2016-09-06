/*
 * CtProtocolState.cpp
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#include <algorithm>
#include <memory>

#include "CtProtocolState.h"
#include "messaging/OverlayMessage.h"
#include "messaging/OverlayTransportMessage.h"
#include "messaging/QueryRequest.h"
#include "messaging/OnionBuilder.h"
#include "util/PathFinder.h"

namespace pddm {

void CtProtocolState::start_query_impl(const messaging::QueryRequest& query_request, const std::vector<FixedPoint_t>& contributed_data) {
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
        //If it's not an encrypted onion, the message will be a PathOverlayMessage
        if(auto path_message = std::dynamic_pointer_cast<messaging::PathOverlayMessage>(overlay_message)) {
            if(path_message->remaining_path.empty()) {
                //The message does not need to be forwarded any more, so it should be received here
                if(protocol_phase == CtProtocolPhase::SHUFFLE) {
                    handle_shuffle_phase_message(*overlay_message);
                } else if(protocol_phase == CtProtocolPhase::ECHO) {
                    handle_echo_phase_message(*overlay_message);
                }
            } //else, it has already been added to waiting_messages
        //If it's an encrypted onion, see if the payload is the next layer or a value that should be recieved here
        } else if(auto enclosed_message = std::dynamic_pointer_cast<messaging::OverlayMessage>(overlay_message->body)){
            waiting_messages.emplace_back(enclosed_message);
        } else {
            //The payload is not an OverlayMessage, so this is the last layer of the onion
            //(ugh, code duplication)
            if(protocol_phase == CtProtocolPhase::SHUFFLE) {
                handle_shuffle_phase_message(*overlay_message);
            } else if(protocol_phase == CtProtocolPhase::ECHO) {
                handle_echo_phase_message(*overlay_message);
            }
        }
    }
    if(message->is_final_message && is_in_overlay_phase()) {
        end_overlay_round();
    }
}

void CtProtocolState::handle_echo_phase_message(const messaging::OverlayMessage& message) {
    if(auto contribution = std::dynamic_pointer_cast<messaging::ValueContribution>(message.body)) {
        if(contribution->value.query_num == my_contribution->query_num) {
            proxy_values.emplace(contribution);
        } else {
            //Log warning
        }
    } else if(message.body != nullptr) {
        //Log warning
    }
}

void CtProtocolState::handle_shuffle_phase_message(const messaging::OverlayMessage& message) {
    //Drop messages that are received in the wrong phase (i.e. not ValueContributions) or have the wrong round number
    if(auto contribution = std::dynamic_pointer_cast<messaging::ValueContribution>(message.body)) {
        if(contribution->value.query_num == my_contribution->query_num) {
            proxy_values.emplace(contribution);
        } else {
            //Log warning
        }
    } else if(message.body != nullptr) {
        //Log warning
    }
}

void CtProtocolState::send_aggregate_if_done() {
    if(aggregation_phase_state->done_receiving_from_children()) {
        aggregation_phase_state->compute_and_send_aggregate(proxy_values);
        protocol_phase = CtProtocolPhase::IDLE;
    }
}

void CtProtocolState::end_overlay_round_impl() {
    //Determine if the Shuffle phase has ended
    if(protocol_phase == CtProtocolPhase::SHUFFLE
            && overlay_round >= FAILURES_TOLERATED + 2 * log2n + 1) {
        //Multicast each received value to its other proxies
        for(const auto& proxy_value : proxy_values) {
            //Remove this meter's ID from the list of proxies
            std::vector<int> other_proxies(proxy_value->value.proxies.size()-1);
            std::remove_copy(proxy_value->value.proxies.begin(),
                    proxy_value->value.proxies.end(), other_proxies.begin(), meter_id);
            auto proxy_paths = util::find_paths(meter_id, other_proxies, num_meters, overlay_round+1);
            for(const auto& proxy_path : proxy_paths) {
                //Encrypt with the destination's public key, but don't make an onion
                outgoing_messages.emplace_back(crypto.rsa_encrypt(std::make_shared<messaging::PathOverlayMessage>(
                        get_current_query_num(), proxy_path, proxy_value), proxy_path.back()));
                //Does proxy_value need to be copied? I don't think so, it won't change
            }
        }
        echo_start_round = overlay_round;
        protocol_phase = CtProtocolPhase::ECHO;
    }
    //Determine if the Echo phase has ended
    else if (protocol_phase == CtProtocolPhase::ECHO
            && overlay_round >= echo_start_round + FAILURES_TOLERATED + 2 * log2n + 1) {

        //Start the Aggregate phase
        protocol_phase = CtProtocolPhase::AGGREGATE;
        start_aggregate_phase();
    }
}

} /* namespace pddm */
