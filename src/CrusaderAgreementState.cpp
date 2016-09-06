/*
 * CrusaderAgreementState.cpp
 *
 *  Created on: May 25, 2016
 *      Author: edward
 */

#include <vector>
#include <memory>
#include <algorithm>

#include "CrusaderAgreementState.h"
#include "Configuration.h"
#include "ConfigurationIncludes.h"
#include "messaging/OverlayMessage.h"
#include "messaging/ValueContribution.h"
#include "messaging/SignedValue.h"
#include "messaging/AgreementValue.h"

namespace pddm {


/**
 * Completes phase 1 of agreement, determining which values to accept,
 * signing the accepted values, and preparing "accept messages" to send
 * to each other node in the agreement group.
 * @param current_round The current round in the peer-to-peer overlay
 *        that messages will be sent over.
 * @return A list of message IDs of accept messages to send to other nodes
 *         in this node's agreement group
 */
std::vector<std::shared_ptr<messaging::OverlayMessage> > CrusaderAgreementState::finish_phase_1(int current_round) {
    std::vector<std::shared_ptr<messaging::OverlayMessage>> accept_messages;
    for(const auto& signed_value_entry : signed_proxy_values) {
        if(signed_value_entry.second.signatures.size() < log2n + 1) {
            //Reject values without enough signatures
            continue;
        }
        //Sign the accepted value
        auto signed_accepted_value = std::make_shared<messaging::AgreementValue>(signed_value_entry.second, node_id);
        crypto_library.rsa_sign(signed_value_entry.second, signed_accepted_value->accepter_signature);
        //Multicast it to the other proxies (besides this node)
        std::vector<int> other_proxies(signed_value_entry.first->value.proxies.size()-1);
        std::remove_copy(signed_value_entry.first->value.proxies.begin(),
                signed_value_entry.first->value.proxies.end(), other_proxies.begin(), node_id);
        auto proxy_paths = util::find_paths(node_id, other_proxies, num_nodes, current_round+1);
        for(const auto& proxy_path : proxy_paths) {
            accept_messages.emplace_back(crypto_library.rsa_encrypt(std::make_shared<messaging::PathOverlayMessage>(
                    query_num, proxy_path, signed_accepted_value), proxy_path.back()));
        }
    }
    phase_1_finished = true;
    return accept_messages;
}

/**
 * Completes phase 2 of agreement, determining which values to accept.
 * @return The list of accepted values.
 */
util::unordered_ptr_set<messaging::ValueContribution> CrusaderAgreementState::finish_phase_2() {
    util::unordered_ptr_set<messaging::ValueContribution> accepted_proxy_values;
    for(const auto& signed_value_entry : signed_proxy_values) {
        //Accept the value if it has enough distinct signatures
        if(signed_value_entry.second.signatures.size() >= log2n + 1) {
            accepted_proxy_values.emplace(signed_value_entry.second.value);
        } else {
            //Log warning
        }
    }
    return accepted_proxy_values;
}

/**
 * Handles a message received during either phase of Crusader Agreement.
 * Determines which phase's logic to use based on the type of the message
 * (specifically, whether message.body is a {@code SignedValue} or an
 * {@code AgreementValue}).
 * @param message A message received during Crusader Agreement.
 */
void CrusaderAgreementState::handle_message(const messaging::OverlayMessage& message) {
    if(auto signed_value = std::dynamic_pointer_cast<messaging::SignedValue>(message.body)) {
        handle_phase_1_message(signed_value);
    } else if (auto agreement_value = std::dynamic_pointer_cast<messaging::AgreementValue>(message.body)) {
        handle_phase_2_message(agreement_value);
    }
}

/**
 * Processes a message for phase 1 of Crusader Agreement: add the signature
 * on this value to the set of received signatures for the same value.
 * @param signed_value A signed value
 */
void CrusaderAgreementState::handle_phase_1_message(const std::shared_ptr<messaging::SignedValue>& signed_value) {
    if(signed_value->signatures.empty()) {
        //Rejected a value without a signature!
        return;
    }
    //The message's signature map should have only one entry in it
    auto signature_pair = *signed_value->signatures.begin();
    if(!crypto_library.rsa_verify(*signed_value->value, signature_pair.second, signature_pair.first)) {
        //Rejected an invalid signature!
        return;
    }
    //If this is the first signature received for the value, put it in the map.
    //Otherwise, add the signature to the list of signatures already in the map.
    auto signed_proxy_values_find = signed_proxy_values.find(signed_value->value);
    if(signed_proxy_values_find == signed_proxy_values.end()) {
        signed_proxy_values[signed_value->value] = *signed_value;
    } else {
        signed_proxy_values_find->second.signatures.insert(signature_pair);
    }
}

/**
 * Processes a message for phase 2 of Crusader Agreement: ensure the
 * received value has enough signatures, and add them to the set of
 * signatures for that value if so.
 * @param agreement_value  A message containing a signed set of signatures
 * for a value.
 */
void CrusaderAgreementState::handle_phase_2_message(const std::shared_ptr<messaging::AgreementValue>& agreement_value) {
    //Verify the sender's signature on the message
    if(!crypto_library.rsa_verify(agreement_value->signed_value, agreement_value->accepter_signature, agreement_value->accepter_id)) {
        //Rejected a message for an invalid signature!
        return;
    }
    //Validate each signature in the package, and remove invalid ones
    int valid_signatures = 0;
    for(auto signatures_iter = agreement_value->signed_value.signatures.begin();
            signatures_iter != agreement_value->signed_value.signatures.end();) {
        //The sender's signature doesn't count towards receiving at least t signatures
        if(signatures_iter->first == agreement_value->accepter_id) {
            ++signatures_iter;
            continue;
        }
        if(crypto_library.rsa_verify(*agreement_value->signed_value.value, signatures_iter->second, signatures_iter->first)) {
            ++valid_signatures;
            ++signatures_iter;
        } else {
            signatures_iter = agreement_value->signed_value.signatures.erase(signatures_iter);
        }

    }

    if(valid_signatures >= log2n) {
        auto signed_proxy_values_find = signed_proxy_values.find(agreement_value->signed_value.value);
        if(signed_proxy_values_find == signed_proxy_values.end()) {
            signed_proxy_values[agreement_value->signed_value.value] = agreement_value->signed_value;
        } else {
            signed_proxy_values_find->second.signatures.insert(agreement_value->signed_value.signatures.begin(),
                    agreement_value->signed_value.signatures.end());
        }
    } else {
        //Log warning
    }
}

} /* namespace pddm */
