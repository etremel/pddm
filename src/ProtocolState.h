/*
 * ProtocolState.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <type_traits>
#include <cmath>
#include <unordered_set>
#include <list>
#include <memory>

#include "Configuration.h"
#include "messaging/QueryRequest.h"
#include "messaging/OverlayTransportMessage.h"
#include "../util/PointerUtil.h"

namespace pddm {

//Forward declaration because MeterClient is defined in terms of ProtocolState
class MeterClient;

template<typename Impl>
class ProtocolState {
        static_assert(std::is_base_of<ProtocolState, Impl>::value, "Template parameter of ProtocolState was not a subclass of ProtocolState!");
    private:
        Impl* impl_this;
        //Methods that must be implemented by the subclass
        bool require_is_in_overlay_phase() const { return impl_this->is_in_overlay_phase(); }
        bool require_is_in_aggregate_phase() const { return impl_this->is_in_aggregate_phase(); }
        void require_send_aggregate_if_done() { impl_this->send_aggregate_if_done(); }
        void require_start_query_impl(const messaging::QueryRequest& query_request,
                const std::vector<FixedPoint_t>& contributed_data) { impl_this->start_query_impl(query_request, contributed_data); }
        void require_handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message) {
            impl_this->handle_overlay_message_impl(message); }
        void require_end_overlay_round_impl() { impl_this->end_overlay_round_impl(); }
        static void require_init_failures_tolerated(const int num_meters) { Impl::init_failures_tolerated(num_meters);}

    public:
        ProtocolState(Impl* subclass_ptr, const NetworkClient_t& network, const CryptoLibrary_t& crypto,
                const TimerManager_t& timer_library, const MeterClient& meter, const int meter_id) :
            impl_this(subclass_ptr), network(network), crypto(crypto), timers(timer_library), meter_id(meter_id), num_meters(meter.get_num_meters()),
            log2n((int) ceil(log2(num_meters))), overlay_round(0), is_last_round(false), round_timeout_timer(-1), ping_response_from_predecessor(false) {}
        virtual ~ProtocolState();

        void start_query(const messaging::QueryRequest& query_request, const std::vector<FixedPoint_t>& contributed_data);
        void handle_overlay_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
        void handle_aggregation_message(const std::shared_ptr<messaging::AggregationMessage>& message);
        void handle_ping_message(const std::shared_ptr<messaging::PingMessage>& message);

        void buffer_future_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
        void buffer_future_message(const std::shared_ptr<messaging::AggregationMessage>& message);

        int get_num_aggregation_groups() const { return num_aggregation_groups; }
        int get_current_query_num() const { return my_contribution ? my_contribution->query_num : -1; }
        int get_current_overlay_round() const { return overlay_round; }

        /** The maximum time (ms) any meter should wait on receiving a message in an overlay round */
        static constexpr int OVERLAY_ROUND_TIMEOUT = 100;
        /** The number of failures tolerated by the currently running instance of the system.
         * This is set only once, at startup, once the number of meters in the system is known.
         * It should be initialized, by calling init_failures_tolerated(), before creating
         * any instances of ProtocolState. */
        static int FAILURES_TOLERATED;

    protected:
        NetworkClient_t& network;
        CryptoLibrary_t& crypto;
        TimerManager_t& timers;
        /** The ID of the meter that this ProtocolState tracks state for */
        int meter_id;
        /** The effective number of meters in the network, including virtual meters */
        int num_meters;
        /** Log (base 2) of num_meters */
        int log2n;
        /** This is a constant, but it must be set by the implementing subclass based on which algorithm it is using. */
        const int num_aggregation_groups;
        int overlay_round;
        bool is_last_round;
        /** The set of meters (by ID) that have definitely failed this round.
         * Meters are added to this set when this meter fails to establish a TCP
         * connection to them, and we don't bother waiting for a message from a
         * meter that has failed. */
        std::set<int> failed_meter_ids;
        /** Handle for the timer registered to timeout the round. */
        int round_timeout_timer;
        bool ping_response_from_predecessor;
        std::list<std::shared_ptr<messaging::OverlayTransportMessage>> future_overlay_messages;
        std::list<std::shared_ptr<messaging::AggregationMessage>> future_aggregation_messages;
        std::list<std::shared_ptr<messaging::OverlayMessage>> waiting_messages;
        std::list<std::shared_ptr<messaging::OverlayMessage>> outgoing_messages;

        std::shared_ptr<messaging::ValueTuple> my_contribution;
        /** This automatically rejects duplicate proxy contributions; it must
         * be an unordered_set because there's no sensible way to "sort" contributions.
         * Since the set of proxies is part of ValueContribution's value equality
         * (by way of ValueTuple), two meters are allowed to contribute the same
         * measurement (they should have distinct proxy sets). */
        util::unordered_ptr_set<messaging::ValueContribution> proxy_values;
        TreeAggregationState aggregation_phase_state;

        void handle_round_timeout();
        inline void end_overlay_round() {
            impl_this->end_overlay_round_impl();
            super_end_overlay_round();
        }
        void super_end_overlay_round();


    private:
        void send_overlay_message_batch();

};

//Useless boilerplate to complete the declaration of the static member FAILURES_TOLERATED
template<typename Impl>
static int ProtocolState<Impl>::FAILURES_TOLERATED;

}

#include "ProtocolState_impl.h"

