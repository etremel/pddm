#include <functional>
#include <algorithm>
#include <queue>
#include <cmath>
#include <spdlog/fmt/ostr.h>

#include "UtilityClient.h"
#include "messaging/StringBody.h"
#include "util/OStreams.h"

namespace pddm {

using std::shared_ptr;
using namespace messaging;

void UtilityClient::handle_message(const std::shared_ptr<messaging::AggregationMessage>& message) {
    logger->trace("Utility received an aggregation message: {}", *message);
    curr_query_results.insert(message);
    //Clear the timeout, since we got a message
    timer_library.cancel_timer(query_timeout_timer);
    //Check if this was definitely the last result from the query
    if(!query_finished &&
            ((query_protocol == QueryProtocol::BFT && (int)curr_query_results.size() > 2 * ProtocolState_t::FAILURES_TOLERATED)
            || (query_protocol != QueryProtocol::BFT && (int)curr_query_results.size() > ProtocolState_t::FAILURES_TOLERATED))) {
        end_query();
    }
    //If the query isn't finished, set a new timeout for the next result message
    if(!query_finished) {
        query_timeout_timer = timer_library.register_timer(query_timeout_time,
                [this](){
                    logger->debug("Utility timed out waiting for query {} after receiving {} messages", query_num, curr_query_results.size());
                    end_query();
        });
    }
}

void UtilityClient::handle_message(const std::shared_ptr<messaging::SignatureRequest>& message) {
    if(curr_query_meters_signed.find(message->sender_id) == curr_query_meters_signed.end()) {
        auto signed_value = crypto_library.rsa_sign_blinded(std::static_pointer_cast<StringBody>(message->body));
        network.send(std::make_shared<messaging::SignatureResponse>(UTILITY_NODE_ID, signed_value), message->sender_id);
        curr_query_meters_signed.insert(message->sender_id);
    }
}

void UtilityClient::start_query(const std::shared_ptr<messaging::QueryRequest>& query) {
    curr_query_meters_signed.clear();
    query_num = query->query_number;
    curr_query_results.clear();
    logger->info("Starting query {}", query_num);
    query_finished = false;
    for(int meter_id = 0; meter_id < num_meters; ++meter_id) {
        network.send(query, meter_id);
    }
    int log2n = std::ceil(std::log2(num_meters));
    int rounds_for_query = 0;
    if(query_protocol == QueryProtocol::BFT) {
        rounds_for_query = 6 * ProtocolState_t::FAILURES_TOLERATED + 3 * log2n * log2n + 3
                + (int) std::ceil(std::log2(num_meters / (double)(2 * ProtocolState_t::FAILURES_TOLERATED + 1)));
    } else if(query_protocol == QueryProtocol::HFT) {
        rounds_for_query = 2 * log2n + 2 * ProtocolState_t::FAILURES_TOLERATED
                + (int) std::ceil(std::log2(num_meters / (double)(ProtocolState_t::FAILURES_TOLERATED + 1)));
    } else if(query_protocol == QueryProtocol::CT) {
        rounds_for_query = 2 * ProtocolState_t::FAILURES_TOLERATED + 4 * log2n + 2
                + (int) std::ceil(std::log2(num_meters / (double)(ProtocolState_t::FAILURES_TOLERATED + 1)));
    }
    query_timeout_timer = timer_library.register_timer(rounds_for_query * NETWORK_ROUNDTRIP_TIMEOUT, [this](){
        logger->debug("Utility timed out waiting for query {} after receiving no messages", query_num);
        end_query();
    });
}

/**
 * This starts the first query in the batch immediately (defined as the one with
 * the lowest query number), but the next one will not start running until the
 * first one completes.
 * @param queries A batch of queries. The order of this vector will be ignored
 * and the queries will be run in order of query number.
 */
void UtilityClient::start_queries(const std::list<std::shared_ptr<messaging::QueryRequest>>& queries) {
    if(queries.empty())
        return;
    //pending_batch_queries.addAll(queries)
    for(const auto& query : queries) pending_batch_queries.push(query);
    shared_ptr<QueryRequest> first_query = pending_batch_queries.top();
    pending_batch_queries.pop();
    start_query(first_query);
}

void UtilityClient::end_query() {
    shared_ptr<AggregationMessageValue> query_result;
    if(query_protocol == QueryProtocol::BFT) {
        for (const auto& result : curr_query_results) {
            logger->debug("Utility results: {}", curr_query_results);
            //Is this the right way to iterate through a multiset and find out the count of each element?
            if((int)curr_query_results.count(result) >= ProtocolState_t::FAILURES_TOLERATED + 1) {
                query_result = result->get_body();
                break;
            }
        }
    } else {
        int most_contributors = 0;
        for (const auto& result : curr_query_results) {
            if(result->get_num_contributors() > most_contributors) {
                query_result = result->get_body();
            }
        }
    }
    curr_query_results.clear();
    if((int) all_query_results.size() <= query_num) {
        all_query_results.resize(query_num+1);
    }
    all_query_results[query_num] = query_result;
    if(query_result == nullptr) {
        logger->error("Query {} failed! No results received by timeout.", query_num);
    } else {
        logger->info("Query {} finished, result was {}", query_num, *query_result);
    }
    query_finished = true;
    for(const auto& callback_pair : query_callbacks) {
        callback_pair.second(query_num, query_result);
    }
    if(!pending_batch_queries.empty()) {
        auto next_query = pending_batch_queries.top();
        pending_batch_queries.pop();
        start_query(next_query);
    }
}

void UtilityClient::listen_loop() {
    network.monitor_incoming_messages();
}

void UtilityClient::shut_down() {
    network.shut_down();
}

/**
 * This allows other components running at the utility to be notified when a
 * query they sent using this UtilityClient (e.g. through start_query) has
 * completed.
 * @param callback A function that will be called every time a query completes.
 * Its arguments will be the query number and the result of that query (a
 * vector of FixedPoint_t).
 * @return A numeric ID that can be used to refer to this callback later.
 */
int UtilityClient::register_query_callback(const QueryCallback& callback) {
    int next_id = query_callbacks.empty() ? 0 : query_callbacks.rbegin()->first + 1;
    query_callbacks.emplace(next_id, callback);
    return next_id;
}

/**
 *
 * @param callback_id The numeric ID of a callback that was previously registered
 * @return True if a callback was deregistered, false if there was no callback
 * with that ID
 */
bool UtilityClient::deregister_query_callback(const int callback_id) {
    int num_removed = query_callbacks.erase(callback_id);
    return num_removed == 1;
}

int UtilityClient::compute_timeout_time(const int num_meters) {
    int messages_for_aggregation = 0;
    if(query_protocol == QueryProtocol::BFT) {
        messages_for_aggregation = (int) std::ceil(std::log2((double) num_meters / (double)(2 * ProtocolState_t::FAILURES_TOLERATED + 1)));
    } else {
        messages_for_aggregation = (int) std::ceil(std::log2((double) num_meters / (double)(ProtocolState_t::FAILURES_TOLERATED + 1)));
    }
    return messages_for_aggregation * NETWORK_ROUNDTRIP_TIMEOUT;
}
} /* namespace psm */

