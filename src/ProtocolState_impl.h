#pragma once

#include "ProtocolState.h"
#include "Configuration.h"
#include "MeterClient.h"

namespace pddm {


template<typename Impl>
void ProtocolState<Impl>::start_query(const messaging::QueryRequest& query_request, const std::vector<FixedPoint_t>& contributed_data) {

impl_this->start_query_impl(query_request, contributed_data);

}

template<typename Impl>
void ProtocolState<Impl>::buffer_future_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message) {
    future_overlay_messages.push_back(message);
}

template<typename Impl>
void ProtocolState<Impl>::buffer_future_message(const std::shared_ptr<messaging::AggregationMessage>& message) {
    future_aggregation_messages.push_back(message);
}

template<typename Impl>
void ProtocolState<Impl>::handle_aggregation_message(const std::shared_ptr<messaging::AggregationMessage>& message) {
    aggregation_phase_state.handle_message(*message);
    impl_this->send_aggregate_if_done();
}


}
