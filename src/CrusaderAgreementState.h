/*
 * CrusaderAgreementState.h
 *
 *  Created on: May 25, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cmath>

#include "Configuration.h"
#include "messaging/SignedValue.h"
#include "messaging/ValueContribution.h"
#include "util/PointerUtil.h"

namespace pddm {
namespace messaging {
struct AgreementValue;
} /* namespace messaging */
} /* namespace pddm */

namespace pddm {

/**
 * State machine for the Crusader Agreement (2-phase Byzantine Agreement) phase
 * of the BFT query protocol. Keeps track of intermediate state of the
 * protocol and handles transitions based on received messages.
 */
class CrusaderAgreementState {
    private:
        const int node_id;
        const int num_nodes;
        const int log2n;
        const int query_num;
        bool phase_1_finished;
        CryptoLibrary_t& crypto_library;
        //I want my map keys to be ValueContributions, but I have to store them by
        //pointer because this map doesn't own them. This mess is the result.
        std::unordered_map<
            std::shared_ptr<messaging::ValueContribution>,
            messaging::SignedValue,
            util::ptr_hash<messaging::ValueContribution>,
            util::ptr_equal<messaging::ValueContribution>
        > signed_proxy_values;
    public:
        CrusaderAgreementState(const int node_id, const int num_nodes, const int query_num, CryptoLibrary_t& crypto_library) :
            node_id(node_id), num_nodes(num_nodes), log2n((int) std::ceil(std::log2(num_nodes))), query_num(query_num),
            phase_1_finished(false), crypto_library(crypto_library) {}

        bool is_phase1_finished() { return phase_1_finished; }
        std::vector<std::shared_ptr<messaging::OverlayMessage>> finish_phase_1(int current_round);
        util::unordered_ptr_set<messaging::ValueContribution> finish_phase_2();
        void handle_message(const messaging::OverlayMessage& message);

    private:
        void handle_phase_1_message(const std::shared_ptr<messaging::SignedValue>& signed_value);
        void handle_phase_2_message(const std::shared_ptr<messaging::AgreementValue>& agreement_value);

};

} /* namespace pddm */

