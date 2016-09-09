/*
 * ProtocolState.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <list>
#include <memory>
#include <set>
#include <vector>

#include "Configuration.h"
#include "FixedPoint_t.h"
#include "messaging/ValueTuple.h"
#include "util/PointerUtil.h"
#include "util/TimerManager.h"
#include "spdlog/spdlog.h"

namespace pddm {
class TreeAggregationState;
namespace messaging {
class AggregationMessage;
class OverlayMessage;
class OverlayTransportMessage;
class PingMessage;
class QueryRequest;
struct ValueContribution;
} /* namespace messaging */
} /* namespace pddm */

namespace pddm {

template<typename Impl>
class ProtocolState {
//        static_assert(std::is_base_of<ProtocolState, Impl>::value, "Template parameter of ProtocolState was not a subclass of ProtocolState!");
    private:
        std::shared_ptr<spdlog::logger> logger;
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
        virtual ~ProtocolState() = default;

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
        ProtocolState(Impl* subclass_ptr, NetworkClient_t& network, CryptoLibrary_t& crypto,
                TimerManager_t& timer_library, const int num_meters, const int meter_id,
                const int num_aggregation_groups);
        ProtocolState(ProtocolState&&) = default;
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
        util::timer_id_t round_timeout_timer;
        bool ping_response_from_predecessor;
        template<typename T> using ptr_list = std::list<std::shared_ptr<T>>;
        ptr_list<messaging::OverlayTransportMessage> future_overlay_messages;
        ptr_list<messaging::AggregationMessage> future_aggregation_messages;
        ptr_list<messaging::OverlayMessage> waiting_messages;
        ptr_list<messaging::OverlayMessage> outgoing_messages;

        std::shared_ptr<messaging::ValueTuple> my_contribution;
        /** This automatically rejects duplicate proxy contributions; it must
         * be an unordered_set because there's no sensible way to "sort" contributions.
         * Since the set of proxies is part of ValueContribution's value equality
         * (by way of ValueTuple), two meters are allowed to contribute the same
         * measurement (they should have distinct proxy sets). */
        util::unordered_ptr_set<messaging::ValueContribution> proxy_values;
        std::unique_ptr<TreeAggregationState> aggregation_phase_state;

        void handle_round_timeout();
        inline void end_overlay_round() {
            impl_this->end_overlay_round_impl();
            super_end_overlay_round();
        }
        void super_end_overlay_round();

        void encrypted_multicast_to_proxies(const std::shared_ptr<messaging::ValueContribution>& contribution);
        void start_aggregate_phase();


    private:
        void send_overlay_message_batch();
};

//Useless boilerplate to complete the declaration of the static member FAILURES_TOLERATED
template<typename Impl>
int ProtocolState<Impl>::FAILURES_TOLERATED;

} /* namespace pddm */

#include "ProtocolState_impl.h"

