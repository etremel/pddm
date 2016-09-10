/*
 * BftProtocolState.cpp
 *
 *  Created on: May 24, 2016
 *      Author: edward
 */

#include <memory>
#include <vector>
#include <cstring>
#include <algorithm>
#include <spdlog/spdlog.h>

#include "BftProtocolState.h"
#include "MeterClient.h"
#include "FixedPoint_t.h"
#include "CrusaderAgreementState.h"
#include "messaging/StringBody.h"
#include "messaging/QueryRequest.h"
#include "messaging/OverlayTransportMessage.h"
#include "messaging/SignatureResponse.h"
#include "messaging/ValueContribution.h"

using namespace pddm::messaging;

namespace pddm {

void BftProtocolState::start_query_impl(const QueryRequest& query_request,
        const std::vector<FixedPoint_t>& contributed_data) {
    protocol_phase = BftProtocolPhase::SETUP;
    accepted_proxy_values.clear();
    agreement_phase_state = std::make_unique<CrusaderAgreementState>(meter_id, num_meters, query_request.query_number, crypto);
    //Encrypt my ValueTuple and send it to the utility to be signed
    auto encrypted_contribution = crypto.rsa_encrypt(my_contribution, meter_id);
    network.send(std::make_shared<messaging::SignatureRequest>(meter_id, encrypted_contribution));
}

void BftProtocolState::handle_signature_response(const std::shared_ptr<SignatureResponse>& message) {
    auto signed_contribution = std::make_shared<ValueContribution>(*my_contribution);
    //Decrypt the utility's signature and copy it into ValueContribution's signature field
    crypto.rsa_decrypt_signature(as_string_pointer(std::static_pointer_cast<SignatureResponse::body_type>(message->body)),
            signed_contribution->signature);

    logger->debug("Meter {} is finished with Setup", meter_id);
    protocol_phase = BftProtocolPhase::SHUFFLE;
    encrypted_multicast_to_proxies(signed_contribution);
}

void BftProtocolState::handle_overlay_message_impl(const std::shared_ptr<OverlayTransportMessage>& message) {
    auto overlay_message = std::static_pointer_cast<OverlayTransportMessage::body_type>(message->body);
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
            if(protocol_phase == BftProtocolPhase::SHUFFLE) {
                handle_shuffle_phase_message(*overlay_message);
            } else if(protocol_phase == BftProtocolPhase::AGREEMENT) {
                handle_agreement_phase_message(*overlay_message);
            }
        } //If destination didn't match, it was already added to waiting_messages
    }
    if(message->is_final_message && is_in_overlay_phase()) {
        end_overlay_round();
    }
}

void BftProtocolState::handle_shuffle_phase_message(const messaging::OverlayMessage& message) {
    //Drop messages that are received in the wrong phase (i.e. not ValueContributions) or have the wrong round number
    if(auto contribution = std::dynamic_pointer_cast<ValueContribution>(message.body)) {
        if(contribution->value.query_num == my_contribution->query_num) {
            //Verify the owner's signature
            if(crypto.rsa_verify(contribution->value, contribution->signature, -1)) {
                proxy_values.emplace(contribution);
                logger->trace("Meter {} received proxy value: {}", meter_id, *contribution);
            }
        } else {
            logger->warn("Meter {} rejected a proxy value because it had the wrong query number: {}", meter_id, *contribution);

        }
    } else if(message.body != nullptr) {
        logger->warn("Meter {} rejected a message because it was not a ValueContribution: {}", meter_id, message);
    }
}

void BftProtocolState::handle_agreement_phase_message(const messaging::OverlayMessage& message) {
    agreement_phase_state->handle_message(message);
}

void BftProtocolState::send_aggregate_if_done() {
    if(aggregation_phase_state->done_receiving_from_children()) {
        aggregation_phase_state->compute_and_send_aggregate(accepted_proxy_values);
        protocol_phase = BftProtocolPhase::IDLE;
        logger->debug("Meter {} is finished with Aggregate", meter_id);
        util::debug_state().num_finished_aggregate++;
        util::print_aggregate_status(logger, num_meters);
    }
}

void BftProtocolState::end_overlay_round_impl() {
    //Determine if the Shuffle phase has ended
    if(protocol_phase == BftProtocolPhase::SHUFFLE
            && overlay_round >= 2 * FAILURES_TOLERATED + log2n * log2n + 1) {
        logger->debug("Meter {} is finished with Shuffle", meter_id);
        //Sign each received value and multicast it to the other proxies
        for(const auto& proxy_value : proxy_values) {
            //Create a SignedValue object to hold this value, and add this node's signature to it
            auto signed_value = std::make_shared<messaging::SignedValue>();
            signed_value->value = proxy_value;
            signed_value->signatures[meter_id].fill(0);
            crypto.rsa_sign(*proxy_value, signed_value->signatures[meter_id]);

            std::vector<int> other_proxies(proxy_value->value.proxies.size()-1);
            std::remove_copy(proxy_value->value.proxies.begin(),
                    proxy_value->value.proxies.end(), other_proxies.begin(), meter_id);
            //Find paths that start at the next round - we send before receive, so we've already sent messages for the current round
            auto proxy_paths = util::find_paths(meter_id, other_proxies, num_meters, overlay_round+1);
            for(const auto& proxy_path : proxy_paths) {
                //Encrypt with the destination's public key, but don't make an onion
                outgoing_messages.emplace_back(crypto.rsa_encrypt(std::make_shared<messaging::PathOverlayMessage>(
                        get_current_query_num(), proxy_path, signed_value), proxy_path.back()));
            }
        }
        agreement_start_round = overlay_round;
        protocol_phase = BftProtocolPhase::AGREEMENT;
        util::debug_state().num_finished_shuffle++;
        util::print_shuffle_status(logger, num_meters);
    }
    //Detect finishing phase 2 of Agreement
    else if(protocol_phase == BftProtocolPhase::AGREEMENT
            && overlay_round >= agreement_start_round + 4 * FAILURES_TOLERATED + 2 * log2n * log2n + 2
            && agreement_phase_state->is_phase1_finished()) {
        logger->debug("Meter {} finished phase 2 of Agreement", meter_id);
        accepted_proxy_values = agreement_phase_state->finish_phase_2();
        util::debug_state().num_finished_agreement++;
        util::print_agreement_status(logger, meter_id, num_meters);

        //Start the Aggregate phase
        protocol_phase = BftProtocolPhase::AGGREGATE;
        start_aggregate_phase();
    }
    //Detect finishing phase 1 of Agreement
    else if(protocol_phase == BftProtocolPhase::AGREEMENT
            && overlay_round >= agreement_start_round + 2 * FAILURES_TOLERATED + log2n * log2n + 1
            && !agreement_phase_state->is_phase1_finished()) {
        logger->debug("Meter {} finished phase 1 of Agreement", meter_id);

        auto accept_messages = agreement_phase_state->finish_phase_1(overlay_round);
        outgoing_messages.insert(outgoing_messages.end(), accept_messages.begin(), accept_messages.end());
    }
}

} /* namespace pddm */

