/*
 * TreeAggregationState.h
 *
 *  Created on: May 24, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <set>

#include "Configuration.h"
#include "messaging/ValueContribution.h"
#include "util/PointerUtil.h"

namespace pddm {
namespace messaging {
class AggregationMessage;
class QueryRequest;
} /* namespace messaging */
} /* namespace pddm */

namespace pddm {

/**
 * Helper class representing the state machine for the Aggregation phase of any
 * subclass of ProtocolState.
 */
class TreeAggregationState {
    private:
        const int node_id;
        const int num_groups;
        const int num_meters;
        NetworkClient_t& network;
        const std::shared_ptr<messaging::QueryRequest> current_query;
        bool initialized;
        int children_received_from;
        int children_needed;
        std::shared_ptr<messaging::AggregationMessage> aggregation_intermediate;
    public:
        TreeAggregationState(const int node_id, const int num_groups, const int num_meters,
                NetworkClient_t& network_client, const std::shared_ptr<messaging::QueryRequest>& query_request) :
            node_id(node_id), num_groups(num_groups), num_meters(num_meters), network(network_client),
            current_query(query_request), initialized(false), children_received_from(0), children_needed(2) {}
        /** Performs initial setup on the tree aggregation state, such as initializing
         * the local intermediate aggregate value to an appropriate zero.*/
        void initialize(const int data_array_length, const std::set<int>& failed_meter_ids);
        bool is_initialized() const { return initialized; }
        bool done_receiving_from_children() const;
        void handle_message(const messaging::AggregationMessage& message);
        void compute_and_send_aggregate(const util::unordered_ptr_set<messaging::ValueContribution>& accepted_proxy_values);
};

} /* namespace pddm */

