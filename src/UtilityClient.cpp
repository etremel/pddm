#include <functional>
#include <algorithm>
#include <queue>
#include <cmath>

#include "UtilityClient.h"
#include "messaging/StringBody.h"

namespace pddm {

using std::shared_ptr;
using namespace messaging;

void UtilityClient::handle_message(const std::shared_ptr<messaging::AggregationMessage>& message) {
    query_results.insert(message);
    //Clear the timeout, since we got a message
    timer_library.cancel_timer(query_timeout_timer);
    //Check if this was definitely the last result from the query
    if(!query_finished &&
            ((query_protocol == QueryProtocol::BFT && query_results.size() > 2 * ProtocolState_t::FAILURES_TOLERATED)
            || (query_protocol != QueryProtocol::BFT && query_results.size() > ProtocolState_t::FAILURES_TOLERATED))) {
        end_query();
    }
    //If the query isn't finished, set a new timeout for the next result message
    if(!query_finished) {
        int messages_for_aggregation = 0;
        if(query_protocol == QueryProtocol::BFT) {
            messages_for_aggregation = (int) std::ceil(std::log2((double) num_meters / (double)(2 * ProtocolState_t::FAILURES_TOLERATED + 1)));
        } else {
            messages_for_aggregation = (int) std::ceil(std::log2((double) num_meters / (double)(ProtocolState_t::FAILURES_TOLERATED + 1)));
        }
        query_timeout_timer = timer_library.register_timer(messages_for_aggregation * NETWORK_ROUNDTRIP_TIMEOUT,
                [this](){end_query();});
    }
}

void UtilityClient::handle_message(const std::shared_ptr<messaging::SignatureRequest>& message) {
    if(curr_query_meters_signed.find(message->sender_id) == curr_query_meters_signed.end()) {
        auto signed_value = crypto_library.rsa_sign_encrypted(std::static_pointer_cast<StringBody>(message->body));
        network.send(std::make_shared<messaging::SignatureResponse>(-1, signed_value), message->sender_id);
        curr_query_meters_signed.insert(message->sender_id);
    }
}

void UtilityClient::start_query(const std::shared_ptr<messaging::QueryRequest>& query) {
    curr_query_meters_signed.clear();
    query_num = query->query_number;
    query_results.clear();
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
    query_timeout_timer = timer_library.register_timer(rounds_for_query * NETWORK_ROUNDTRIP_TIMEOUT, [this](){end_query();});
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
        for (const auto& result : query_results) {
            //Is this the right way to iterate through a multiset and find out the count of each element?
            if(query_results.count(result) >= ProtocolState_t::FAILURES_TOLERATED + 1) {
                query_result = result->get_body();
                break;
            }
        }
    } else {
        int most_contributors = 0;
        for (const auto& result : query_results) {
            if(result->get_num_contributors() > most_contributors) {
                query_result = result->get_body();
            }
        }
    }
    query_results.clear();
    if(all_query_results.size() < query_num) {
        all_query_results.resize(query_num+1);
    }
    all_query_results[query_num] = query_result;
    query_finished = true;
    if(!pending_batch_queries.empty()) {
        auto next_query = pending_batch_queries.top();
        pending_batch_queries.pop();
        start_query(next_query);
    }
}

} /* namespace psm */
