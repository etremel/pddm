/*
 * HftProtocolState.h
 *
 *  Created on: May 26, 2016
 *      Author: edward
 */

#pragma once
#include <cmath>

#include "ProtocolState.h"
#include "util/PointerUtil.h"

namespace pddm {

enum class HftProtocolPhase { IDLE, SCATTER, GATHER, AGGREGATE };

class HftProtocolState: public ProtocolState<HftProtocolState> {
    private:
        HftProtocolPhase protocol_phase;
        int gather_start_round;
        util::unordered_ptr_set<messaging::OverlayMessage> current_flood_messages;
        util::unordered_ptr_set<messaging::OverlayMessage> relay_messages;
    public:
        HftProtocolState(const NetworkClient_t& network, const CryptoLibrary_t& crypto,
                const TimerManager_t& timer_library, const MeterClient& meter, const int meter_id) :
                    ProtocolState(this, network, crypto, timer_library, meter, meter_id),
                    num_aggregation_groups(FAILURES_TOLERATED + 1),
                    protocol_phase(HftProtocolPhase::IDLE), gather_start_round(0) {}
        virtual ~HftProtocolState();

        bool is_in_overlay_phase() const { return protocol_phase == HftProtocolPhase::SCATTER || protocol_phase == HftProtocolPhase::GATHER; }
        bool is_in_aggregate_phase() const { return protocol_phase == HftProtocolPhase::AGGREGATE; }

        static void init_failures_tolerated(const int num_meters) {
            FAILURES_TOLERATED = (int) round(num_meters * 0.1);
        }

    protected:
        void send_aggregate_if_done();
        void end_overlay_round_impl();
        void start_query_impl(const messaging::QueryRequest& query_request, const std::vector<FixedPoint_t>& contributed_data);
        void handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
};

} /* namespace pddm */

