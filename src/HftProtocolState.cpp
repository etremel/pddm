/*
 * HftProtocolState.cpp
 *
 *  Created on: May 26, 2016
 *      Author: edward
 */

#include "HftProtocolState.h"

namespace pddm {

void HftProtocolState::send_aggregate_if_done() {
}

void HftProtocolState::end_overlay_round_impl() {
}

void HftProtocolState::start_query_impl(const messaging::QueryRequest& query_request, const std::vector<FixedPoint_t>& contributed_data) {
}

void HftProtocolState::handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message) {
}


} /* namespace pddm */

