/*
 * TreeAggregationState.h
 *
 *  Created on: May 24, 2016
 *      Author: edward
 */

#pragma once

namespace pddm {

#include <set>

//#include "Configuration.h"
#include "messaging/AggregationMessage.h"
#include "messaging/QueryRequest.h"

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
        const messaging::QueryRequest& current_query;
        bool initialized;
        int children_received_from;
        int children_needed;
        messaging::AggregationMessage aggregation_intermediate;
    public:
        TreeAggregationState(const int node_id, const int num_groups, const int num_meters,
                const NetworkClient_t& network_client, const messaging::QueryRequest& query_request) :
            node_id(node_id), num_meters(num_meters), num_groups(num_groups), network(network_client),
            current_query(query_request), initialized(false), children_received_from(0), children_needed(2) {}
        /** Performs initial setup on the tree aggregation state, such as initializing
         * the local intermediate aggregate value to an appropriate zero.*/
        void initialize(const int data_array_length, const std::set<int>& failed_meter_ids);
        bool is_initialized() const { return initialized; }
        bool done_receiving_from_children() const;
        void handle_message(const messaging::AggregationMessage& message);
        void compute_and_send_aggregate(const std::set<messaging::ValueContribution>& accepted_proxy_values);
};

} /* namespace psm */

