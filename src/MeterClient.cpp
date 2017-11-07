/*
 * MeterClient.cpp
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#include <type_traits>
#include <memory>
#include <vector>


#include "MeterClient.h"
#include "messaging/QueryRequest.h"
#include "messaging/OverlayMessage.h"
#include "messaging/OverlayTransportMessage.h"
#include "messaging/AggregationMessage.h"
#include "messaging/SignatureRequest.h"
#include "messaging/SignatureResponse.h"
#include "messaging/PingMessage.h"
#include "util/Overlay.h"

#include "BftProtocolState.h" //I need to include this even if Configuration is set not to use BftProtocolState :(

using std::shared_ptr;
using namespace pddm::messaging;

namespace pddm {


void MeterClient::set_second_id(const int id) {
    second_id = id;
    has_second_id = true;
    secondary_protocol_state.emplace(network_client, crypto_library, timer_library, num_meters, id);
}


void MeterClient::handle_message(const std::shared_ptr<messaging::AggregationMessage>& message) {
    if(util::aggregation_group_for(message->sender_id, primary_protocol_state.get_num_aggregation_groups(), num_meters) ==
            util::aggregation_group_for(meter_id, primary_protocol_state.get_num_aggregation_groups(), num_meters)) {
        if(primary_protocol_state.is_in_aggregate_phase()) {
            primary_protocol_state.handle_aggregation_message(message);
        //If it's a message for the right query, but I received it too early, buffer it for the future
        } else if (message->query_num == primary_protocol_state.get_current_query_num()) {
            primary_protocol_state.buffer_future_message(message);
        } else {
            logger->warn("Meter {} rejected a message from meter {} with the wrong query number: {}", meter_id, message->sender_id, *message);
        }
    } else if (has_second_id &&
            util::aggregation_group_for(message->sender_id, secondary_protocol_state->get_num_aggregation_groups(), num_meters) ==
                    util::aggregation_group_for(second_id, secondary_protocol_state->get_num_aggregation_groups(), num_meters)) {
        if(secondary_protocol_state->is_in_aggregate_phase()) {
            secondary_protocol_state->handle_aggregation_message(message);
        } else if(message->query_num == secondary_protocol_state->get_current_query_num()) {
            secondary_protocol_state->buffer_future_message(message);
        } else {
            logger->warn("Meter {} rejected a message from meter {} with the wrong query number: {}", second_id, message->sender_id, *message);
        }
    }
}

void MeterClient::handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message) {
    if(util::gossip_target(message->sender_id, message->sender_round, num_meters) == meter_id) {
        std::shared_ptr<OverlayMessage> wrapped_message = std::static_pointer_cast<OverlayMessage>(message->body);
        if(wrapped_message->query_num > primary_protocol_state.get_current_query_num()) {
            //If the message is for a future query, buffer it until I get the query-start message
            primary_protocol_state.buffer_future_message(message);
        } else if (wrapped_message->query_num < primary_protocol_state.get_current_query_num()) {
            logger->warn("Meter {} discarded an obsolete message from meter {} for an old query: {}", meter_id, message->sender_id, *message);
        //At this point, we know the message is for the current query
        } else if(message->sender_round == primary_protocol_state.get_current_overlay_round()) {
            primary_protocol_state.handle_overlay_message(message);
        } else if(message->sender_round > primary_protocol_state.get_current_overlay_round()) {
            //If it's a message for a future round, buffer it until my round advances
            primary_protocol_state.buffer_future_message(message);
        } else {
            logger->debug("Meter {}, already in round {}, rejected a message from meter {} as too old: {}", meter_id, primary_protocol_state.get_current_overlay_round(), message->sender_id, *message);
        }
    //Same handling but for messages intended for my second ID
    } else if (has_second_id &&
            util::gossip_target(message->sender_id, message->sender_round, num_meters) == second_id) {
        std::shared_ptr<OverlayMessage> wrapped_message = std::static_pointer_cast<OverlayMessage>(message->body);
        if(wrapped_message->query_num > secondary_protocol_state->get_current_query_num()) {
            secondary_protocol_state->buffer_future_message(message);
        } else if (wrapped_message->query_num < secondary_protocol_state->get_current_query_num()) {
            logger->warn("Meter {} discarded an obsolete message from meter {} for an old query: {}", second_id, message->sender_id, *message);
        } else if(message->sender_round == secondary_protocol_state->get_current_overlay_round()) {
            secondary_protocol_state->handle_overlay_message(message);
        } else if(message->sender_round > secondary_protocol_state->get_current_overlay_round()) {
            secondary_protocol_state->buffer_future_message(message);
        } else {
            logger->debug("Meter {} rejected a message from meter {} as too old: {}", second_id, message->sender_id, *message);
        }
    } else {
        logger->warn("Meter {} rejected a message because it has the wrong gossip target: {}", meter_id, *message);
    }
}


void MeterClient::handle_message(const std::shared_ptr<messaging::PingMessage>& message) {
    primary_protocol_state.handle_ping_message(message);
    if(has_second_id)
        secondary_protocol_state->handle_ping_message(message);
}

/**
 * Starts the data collection protocol
 * @param message The query request message received from the utility
 */
void MeterClient::handle_message(const std::shared_ptr<messaging::QueryRequest>& message) {
    using namespace messaging;
    std::vector<FixedPoint_t> contributed_data;
    if(message->request_type == QueryType::CURR_USAGE_SUM || message->request_type == QueryType::CURR_USAGE_HISTOGRAM) {
        contributed_data.emplace_back(meter->measure_consumption(message->time_window));
    } else if (message->request_type == QueryType::PROJECTED_SUM || message->request_type == QueryType::PROJECTED_HISTOGRAM) {
        contributed_data = meter->simulate_projected_usage(message->proposed_price_function, message->time_window);
    } else if (message->request_type == QueryType::CUMULATIVE_USAGE) {
         contributed_data.emplace_back(meter->measure_daily_consumption());
    } else if (message->request_type == QueryType::AVAILABLE_OFFSET_BREAKDOWN) {
        contributed_data.resize(2);
        contributed_data[0] = meter->measure_consumption(message->time_window);
        contributed_data[1] = meter->measure_shiftable_consumption(message->time_window);
    } else {
        logger->error("Meter {} received a message with unknown query type!", meter_id);
    }
    primary_protocol_state.start_query(message, contributed_data);
    if(has_second_id) {
        secondary_protocol_state->start_query(message, std::vector<FixedPoint_t>{});
    }
}

void MeterClient::handle_message(const std::shared_ptr<messaging::SignatureResponse>& message) {
    //Using a raw pointer to a local value type is ugly, but it's the only way to select code at compile time based on the type of ProtocolState_t
    handle_signature_response(message, &primary_protocol_state);
}

void MeterClient::handle_signature_response(const std::shared_ptr<messaging::SignatureResponse>& message, BftProtocolState* bft_protocol) {
    bft_protocol->handle_signature_response(message);
}

void MeterClient::handle_signature_response(const std::shared_ptr<messaging::SignatureResponse>& message, void* protocol_is_not_bft) {
    /* Do nothing, non-BFT protocols will never get this message */
}

void MeterClient::main_loop() {
    network_client.monitor_incoming_messages();
}

void MeterClient::shut_down() {
    network_client.shut_down();
}

} /* namespace pddm */
