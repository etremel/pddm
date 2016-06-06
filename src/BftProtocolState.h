/*
 * BftProtocolState.h
 *
 *  Created on: May 24, 2016
 *      Author: edward
 */

#pragma once

#include <cmath>

#include "ProtocolState.h"
#include "messaging/SignatureResponse.h"

namespace pddm {

//Forward declaration because MeterClient is defined in terms of ProtocolState
class MeterClient;

enum class BftProtocolPhase { IDLE, SETUP, SHUFFLE, AGREEMENT, AGGREGATE };

class BftProtocolState: public ProtocolState<BftProtocolState> {
    private:
        BftProtocolPhase protocol_phase;
        CrusaderAgreementState agreement_phase_state;
        int agreement_start_round;
        std::vector<std::shared_ptr<messaging::ValueContribution>> accepted_proxy_values;
    public:
        BftProtocolState(const NetworkClient_t& network, const CryptoLibrary_t& crypto,
                const TimerManager_t& timer_library, const MeterClient& meter, const int meter_id) :
                    ProtocolState(this, network, crypto, timer_library, meter, meter_id),
                    num_aggregation_groups(2 * FAILURES_TOLERATED + 1), protocol_phase(BftProtocolPhase::IDLE),
                    agreement_start_round(0) {}
        virtual ~BftProtocolState();
        void handle_signature_response(const std::shared_ptr<messaging::SignatureResponse>& message);

        bool is_in_overlay_phase() const { return protocol_phase == BftProtocolPhase::SHUFFLE || protocol_phase == BftProtocolPhase::AGREEMENT; }
        bool is_in_aggregate_phase() const { return protocol_phase == BftProtocolPhase::AGGREGATE; }

        static void init_failures_tolerated(const int num_meters) {
            FAILURES_TOLERATED = (int) ceil(log2(num_meters));
        }

    protected:
        void send_aggregate_if_done();
        void end_overlay_round_impl();
        void start_query_impl(const messaging::QueryRequest& query_request, const std::vector<FixedPoint_t>& contributed_data);
        void handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
};

} /* namespace psm */

