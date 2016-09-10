/*
 * BftProtocolState.h
 *
 *  Created on: May 24, 2016
 *      Author: edward
 */

#pragma once

#include <cmath>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>

#include "ProtocolState.h"
#include "FixedPoint_t.h"
#include "messaging/QueryRequest.h"
#include "messaging/OverlayTransportMessage.h"
#include "messaging/SignatureResponse.h"
#include "messaging/ValueContribution.h"
#include "CrusaderAgreementState.h"
#include "util/PointerUtil.h"

namespace pddm {


enum class BftProtocolPhase { IDLE, SETUP, SHUFFLE, AGREEMENT, AGGREGATE };

class BftProtocolState: public ProtocolState<BftProtocolState> {
    private:
        std::shared_ptr<spdlog::logger> logger;
        BftProtocolPhase protocol_phase;
        std::unique_ptr<CrusaderAgreementState> agreement_phase_state;
        int agreement_start_round;
        util::unordered_ptr_set<messaging::ValueContribution> accepted_proxy_values;
        void handle_agreement_phase_message(const messaging::OverlayMessage& message);
        void handle_shuffle_phase_message(const messaging::OverlayMessage& message);
    public:
        BftProtocolState(NetworkClient_t& network, CryptoLibrary_t& crypto,
                TimerManager_t& timer_library, const int num_meters, const int meter_id) :
                    ProtocolState(this, network, crypto, timer_library, num_meters, meter_id, 2 * FAILURES_TOLERATED + 1),
                    logger(spdlog::get("global_logger")),
                    protocol_phase(BftProtocolPhase::IDLE),
                    agreement_start_round(0) {}
        virtual ~BftProtocolState() = default;
        void handle_signature_response(const std::shared_ptr<messaging::SignatureResponse>& message);

        bool is_in_overlay_phase() const { return protocol_phase == BftProtocolPhase::SHUFFLE || protocol_phase == BftProtocolPhase::AGREEMENT; }
        bool is_in_aggregate_phase() const { return protocol_phase == BftProtocolPhase::AGGREGATE; }

        static void init_failures_tolerated(const int num_meters) {
            FAILURES_TOLERATED = (int) std::ceil(std::log2(num_meters));
        }

    protected:
        void send_aggregate_if_done();
        void end_overlay_round_impl();
        void start_query_impl(const messaging::QueryRequest& query_request, const std::vector<FixedPoint_t>& contributed_data);
        void handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message);

        friend class ProtocolState;
};

} /* namespace psm */

