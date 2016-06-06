/*
 * CtProtocolState.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include "Configuration.h"
#include "ProtocolState.h"

namespace pddm {

enum class CtProtocolPhase { IDLE, SHUFFLE, ECHO, AGGREGATE };

class CtProtocolState: public ProtocolState<CtProtocolState> {
    private:
        int echo_start_round;
        CtProtocolPhase protocol_phase;
        void handle_echo_phase_message(const messaging::OverlayMessage& message);
        void handle_shuffle_phase_message(const messaging::OverlayMessage& message);

    public:
        CtProtocolState(const NetworkClient_t& network, const CryptoLibrary_t& crypto, const TimerManager_t& timer_library,
                const MeterClient& meter, const int meter_id) :
            ProtocolState(this, network, crypto, timer_library, meter, meter_id),
            num_aggregation_groups(FAILURES_TOLERATED + 1), protocol_phase(CtProtocolPhase::IDLE) {};
        virtual ~CtProtocolState();

        bool is_in_overlay_phase() const { return protocol_phase == CtProtocolPhase::SHUFFLE || protocol_phase == CtProtocolPhase::ECHO; }
        bool is_in_aggregate_phase() const { return protocol_phase == CtProtocolPhase::AGGREGATE; }

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
