/*
 * HftProtocolState.cpp
 *
 *  Created on: May 26, 2016
 *      Author: edward
 */

#include "HftProtocolState.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "messaging/OverlayMessage.h"
#include "messaging/OverlayTransportMessage.h"
#include "messaging/QueryRequest.h"
#include "messaging/ValueContribution.h"
#include "messaging/ValueTuple.h"
#include "TreeAggregationState.h"
#include "util/Overlay.h"
#include "util/OStreams.h"
#include "util/DebugState.h"

namespace pddm {

void HftProtocolState::start_query_impl(const std::shared_ptr<messaging::QueryRequest>& query_request, const std::vector<FixedPoint_t>& contributed_data) {
    protocol_phase = HftProtocolPhase::SCATTER;
    current_flood_messages.clear();
    relay_messages.clear();
    gather_start_round = 0;

    //We don't actually need to sign contributions in crash-tolerant mode, but use ValueContribution anyway
    auto signed_contribution = std::make_shared<messaging::ValueContribution>(*my_contribution);

    //Pick relay nodes for each proxy, selecting uniformly at random from non-proxy nodes
    std::vector<int> id_range(num_meters);
    std::iota(id_range.begin(), id_range.end(), 0);
    std::vector<int> non_proxies;
    std::sort(my_contribution->proxies.begin(), my_contribution->proxies.end());
    std::set_intersection(id_range.begin(), id_range.end(),
            my_contribution->proxies.begin(), my_contribution->proxies.end(),
            std::back_inserter(non_proxies));
    std::shuffle(non_proxies.begin(), non_proxies.end(), random_engine);
    std::map<int, int> relays;
    for(int i = 0; i < my_contribution->proxies.size(); ++i) {
        relays[my_contribution->proxies[i]] = non_proxies[i];
    }
    //Create a two-layer onion for each tuple, and start flooding it towards the relay
    for(const auto& proxy_relay : relays) {
        auto inner_layer = crypto.rsa_encrypt(std::make_shared<messaging::OverlayMessage>(
                query_request->query_number, proxy_relay.first, signed_contribution, true),
                proxy_relay.first);
        auto outer_layer = crypto.rsa_encrypt(std::make_shared<messaging::OverlayMessage>(
                query_request->query_number, proxy_relay.second, inner_layer, true),
                proxy_relay.second);
        current_flood_messages.emplace(outer_layer);
    }
    //Start the overlay by ending "round -1", which will send the messages at the start of round 0
    end_overlay_round();
}



void HftProtocolState::handle_overlay_message_impl(const std::shared_ptr<messaging::OverlayTransportMessage>& message) {
    auto overlay_message = std::static_pointer_cast<messaging::OverlayMessage>(message->body);
    //Dummy messages will have a null payload
    if(overlay_message->body != nullptr) {
        //With this protocol there are no Path messages, only flood messages
        if(overlay_message->destination == meter_id) {
            if(protocol_phase == HftProtocolPhase::SCATTER) {
                handle_scatter_phase_message(*overlay_message);
            } else if(protocol_phase == HftProtocolPhase::GATHER) {
                handle_gather_phase_message(*overlay_message);
            }
        } else {
            //Messages not destined for me must continue to get sent on every round
            current_flood_messages.emplace(overlay_message);
        }
    }
    if(message->is_final_message && is_in_overlay_phase()) {
        end_overlay_round();
    }
}

void HftProtocolState::handle_scatter_phase_message(const messaging::OverlayMessage& message) {
    if(auto wrapped_message = std::dynamic_pointer_cast<messaging::OverlayMessage>(message.body)) {
        //Just unwrap the message and save it for Gather
        logger->debug("Meter {} got a relay message: {}", meter_id, *wrapped_message);
        relay_messages.emplace(wrapped_message);
    } else {
        logger->warn("Meter {} rejected a message because it was not a tuple to relay: {}", meter_id, message);
    }
}

void HftProtocolState::handle_gather_phase_message(const messaging::OverlayMessage& message) {
    if(auto contribution = std::dynamic_pointer_cast<messaging::ValueContribution>(message.body)) {
        if(contribution->value.query_num == my_contribution->query_num) {
            logger->debug("Meter {} received proxy value: {}", meter_id, *contribution);
            proxy_values.emplace(contribution);
        } else {
            logger->warn("Meter {} rejected a proxy value because it had the wrong query number: {}", meter_id, *contribution);
        }
    } else if(message.body != nullptr) {
        logger->warn("Meter {} rejected a message because it was not a ValueContribution: {}", meter_id, message);
    }
}

void HftProtocolState::end_overlay_round_impl() {
    //Determine if the Scatter phase has ended
    if(protocol_phase == HftProtocolPhase::SCATTER
            && overlay_round >= log2n + FAILURES_TOLERATED) {
        logger->debug("Meter {} is finished with Scatter", meter_id);
        //Discard flood messages for the Scatter phase
        current_flood_messages.clear();
        //Start flooding each relay message to its proxy
        std::swap(current_flood_messages, relay_messages);

        gather_start_round = overlay_round;
        protocol_phase = HftProtocolPhase::GATHER;
        util::debug_state().num_finished_scatter++;
        util::print_scatter_status(logger, num_meters);
    }
    //Determine if the Gather phase has ended
    else if(protocol_phase == HftProtocolPhase::GATHER
            && overlay_round >= gather_start_round + log2n + FAILURES_TOLERATED) {
        logger->debug("Meter {} is finished with Gather", meter_id);
        util::debug_state().num_finished_gather++;
        util::print_gather_status(logger, meter_id, num_meters);
        current_flood_messages.clear();

        //Start the Aggregate phase
        protocol_phase = HftProtocolPhase::AGGREGATE;
        start_aggregate_phase();
    }

    //If we're still in an overlay-using phase, send all current flood messages
    if(is_in_overlay_phase()) {
        for(auto flood_message_iter = current_flood_messages.begin();
                flood_message_iter != current_flood_messages.end(); ) {
            outgoing_messages.emplace_back(*flood_message_iter);
            //If the message will be sent to its final destination, it's now
            //safe to remove it from current_flood_messages
            if(util::gossip_target(meter_id, overlay_round+1, num_meters) == (*flood_message_iter)->destination) {
                flood_message_iter = current_flood_messages.erase(flood_message_iter);
            } else {
                ++flood_message_iter;
            }
        }
    }
}

void HftProtocolState::send_aggregate_if_done() {
    if(aggregation_phase_state->done_receiving_from_children()) {
        aggregation_phase_state->compute_and_send_aggregate(proxy_values);
        protocol_phase = HftProtocolPhase::IDLE;
        current_flood_messages.clear();
        logger->debug("Meter {} is finished with Aggregate", meter_id);
        util::debug_state().num_finished_aggregate++;
        util::print_aggregate_status(logger, num_meters);
    }
}


} /* namespace pddm */

