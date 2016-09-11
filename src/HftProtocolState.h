/*
 * HftProtocolState.h
 *
 *  Created on: May 26, 2016
 *      Author: edward
 */

#pragma once
#include <cmath>
#include <vector>
#include <memory>
#include <spdlog/spdlog.h>

#include "ProtocolState.h"
#include "FixedPoint_t.h"
#include "util/PointerUtil.h"

namespace pddm {

enum class HftProtocolPhase { IDLE, SCATTER, GATHER, AGGREGATE };

class HftProtocolState: public ProtocolState<HftProtocolState> {
    private:
        std::shared_ptr<spdlog::logger> logger;
        HftProtocolPhase protocol_phase;
        int gather_start_round;
        util::unordered_ptr_set<messaging::OverlayMessage> current_flood_messages;
        util::unordered_ptr_set<messaging::OverlayMessage> relay_messages;
        std::mt19937 random_engine;
        void handle_scatter_phase_message(const messaging::OverlayMessage& message);
        void handle_gather_phase_message(const messaging::OverlayMessage& message);
    public:
        HftProtocolState(NetworkClient_t& network, CryptoLibrary_t& crypto,
                TimerManager_t& timer_library, const int num_meters, const int meter_id) :
                    ProtocolState(this, network, crypto, timer_library, num_meters, meter_id, FAILURES_TOLERATED + 1),
                    logger(spdlog::get("global_logger")),
                    protocol_phase(HftProtocolPhase::IDLE),
                    gather_start_round(0) {}
        HftProtocolState(HftProtocolState&&) = default;
        virtual ~HftProtocolState() = default;

        bool is_in_overlay_phase() const { return protocol_phase == HftProtocolPhase::SCATTER || protocol_phase == HftProtocolPhase::GATHER; }
        bool is_in_aggregate_phase() const { return protocol_phase == HftProtocolPhase::AGGREGATE; }

        static void init_failures_tolerated(const int num_meters) {
            FAILURES_TOLERATED = (int) std::round(num_meters * 0.1f);
        }

    protected:
        void send_aggregate_if_done();
        void end_overlay_round_impl();
        void start_query_impl(const std::shared_ptr<messaging::QueryRequest>& query_request, const std::vector<FixedPoint_t>& contributed_data);
        void handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message);

        friend class ProtocolState;
};

} /* namespace pddm */

