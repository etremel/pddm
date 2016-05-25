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

//Forward declaration because MeterClient is defined in terms of ProtocolState
class MeterClient;

enum class CtProtocolPhase { IDLE, SHUFFLE, ECHO, AGGREGATE };

class CtProtocolState: public ProtocolState<CtProtocolState> {
    private:
        int echo_start_round;
        CtProtocolPhase aggregation_phase;
        void handle_echo_phase_message(const messaging::OverlayMessage& message);

    public:
        CtProtocolState(const NetworkClient_t& network, const CryptoLibrary_t& crypto, const TimerManager_t& timer_library,
                const MeterClient& meter, const int meter_id) :
            ProtocolState(this, network, crypto, timer_library, meter, meter_id) {};
        virtual ~CtProtocolState();
        void start_query_impl(const messaging::QueryRequest& query_request, const std::vector<FixedPoint_t>& contributed_data);
        void handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message);


        bool is_in_overlay_phase() const { return aggregation_phase == CtProtocolPhase::SHUFFLE || aggregation_phase == CtProtocolPhase::ECHO; }
        bool is_in_aggregate_phase() const { return aggregation_phase == CtProtocolPhase::AGGREGATE; }

    protected:
        void send_aggregate_if_done();
        void end_overlay_round_impl();
};

} /* namespace psm */
