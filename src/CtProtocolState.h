/*
 * CtProtocolState.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <vector>
#include <cmath>

#include "Configuration.h"
#include "ProtocolState.h"
#include "FixedPoint_t.h"
#include "ConfigurationIncludes.h"
#include "spdlog/spdlog.h"

namespace pddm {

enum class CtProtocolPhase { IDLE, SHUFFLE, ECHO, AGGREGATE };

class CtProtocolState: public ProtocolState<CtProtocolState> {
    private:
        std::shared_ptr<spdlog::logger> logger;
        int echo_start_round;
        CtProtocolPhase protocol_phase;
        void handle_echo_phase_message(const messaging::OverlayMessage& message);
        void handle_shuffle_phase_message(const messaging::OverlayMessage& message);

    public:
        CtProtocolState(NetworkClient_t& network, CryptoLibrary_t& crypto, TimerManager_t& timer_library,
                const int num_meters, const int meter_id) :
            ProtocolState(this, network, crypto, timer_library, num_meters, meter_id, FAILURES_TOLERATED + 1),
            logger(spdlog::get("global_logger")),
            echo_start_round(0),
            protocol_phase(CtProtocolPhase::IDLE) {};
        CtProtocolState(CtProtocolState&&) = default;
        virtual ~CtProtocolState() = default;

        bool is_in_overlay_phase() const { return protocol_phase == CtProtocolPhase::SHUFFLE || protocol_phase == CtProtocolPhase::ECHO; }
        bool is_in_aggregate_phase() const { return protocol_phase == CtProtocolPhase::AGGREGATE; }

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
