/*
 * TreeAggregationState.cpp
 *
 *  Created on: May 24, 2016
 *      Author: edward
 */

#include <memory>

#include "TreeAggregationState.h"
#include "util/Overlay.h"

namespace pddm {

void TreeAggregationState::initialize(const int data_array_length, const std::set<int>& failed_meter_ids) {
    aggregation_intermediate = messaging::AggregationMessage(node_id, current_query.query_number,
            std::make_shared<messaging::AggregationMessageValue>(data_array_length));
    children_received_from = 0;
    std::pair<int, int> children = util::aggregation_tree_children(node_id, num_groups, num_meters);
    children_needed = 2;
    //                          "failed_meter_ids contains children.first"
    if (children.first == -1 || failed_meter_ids.find(children.first) != failed_meter_ids.end()) {
        children_needed--;
    }
    if (children.second == -1 || failed_meter_ids.find(children.second) != failed_meter_ids.end()) {
        children_needed--;
    }
    initialized = true;
}

bool TreeAggregationState::done_receiving_from_children() const {
    return children_received_from >= children_needed;
}

void TreeAggregationState::handle_message(const messaging::AggregationMessage& message) {
}

void TreeAggregationState::compute_and_send_aggregate(const std::set<messaging::ValueContribution>& accepted_proxy_values) {
}

} /* namespace psm */
