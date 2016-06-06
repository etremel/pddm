#include <functional>
#include <algorithm>
#include <queue>
#include <cmath>

#include "UtilityClient.h"

namespace pddm {

using std::shared_ptr;
using namespace messaging;

/* Instead of making three subclasses of UtilityClient, we'll just switch behavior
 * based on which subclass of ProtocolState we're using. This statically initializes
 * the value of query_protocol based on which type ProtocolState_t is mapped to. */
UtilityClient::QueryProtocol UtilityClient::query_protocol = std::is_same<ProtocolState_t, BftProtocolState>::value ?
        QueryProtocol::BFT :
        (std::is_same<ProtocolState_t, HftProtocolState>::value ?
                QueryProtocol::HFT : QueryProtocol::CT);

void UtilityClient::handle_message(const std::shared_ptr<messaging::AggregationMessage>& message) {
    query_results.insert(message);
    //Clear the timeout, since we got a message
    timer_library.cancel_timer(query_timeout_timer);
    //Check if this was definitely the last result from the query
    if(!query_finished && (query_protocol == QueryProtocol::BFT && query_results.size() > 2 * ProtocolState_t::FAILURES_TOLERATED)
            || (query_protocol != QueryProtocol::BFT && query_results.size() > ProtocolState_t::FAILURES_TOLERATED)) {
        end_query();
    }
    //If the query isn't finished, set a new timeout for the next result message
    if(!query_finished) {
        int messages_for_aggregation = 0;
        if(query_protocol == QueryProtocol::BFT) {
            messages_for_aggregation = (int) ceil(log2(num_meters / (double)(2 * ProtocolState_t::FAILURES_TOLERATED + 1)));
        } else {
            messages_for_aggregation = (int) ceil(log2(num_meters / (double)(ProtocolState_t::FAILURES_TOLERATED + 1)));
        }
        query_timeout_timer = timer_library.register_timer(messages_for_aggregation * NETWORK_ROUNDTRIP_TIMEOUT,
                std::bind(&UtilityClient::end_query, this));
    }
}

void UtilityClient::handle_message(const std::shared_ptr<messaging::SignatureRequest>& message) {
    if(curr_query_meters_signed.find(message->sender_id) == curr_query_meters_signed.end()) {
        //Need a cryptoLibrary method to sign the encrypted message in the signature request
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
    int log2n = ceil(log2(num_meters));
    int rounds_for_query = 0;
    if(query_protocol == QueryProtocol::BFT) {
        rounds_for_query = 6 * ProtocolState_t::FAILURES_TOLERATED + 3 * log2n * log2n + 3
                + (int) ceil(log2(num_meters / (double)(2 * ProtocolState_t::FAILURES_TOLERATED + 1)));
    } else if(query_protocol == QueryProtocol::HFT) {
        rounds_for_query = 2 * log2n + 2 * ProtocolState_t::FAILURES_TOLERATED
                + (int) ceil(log2(num_meters / (double)(ProtocolState_t::FAILURES_TOLERATED + 1)));
    } else if(query_protocol == QueryProtocol::CT) {
        rounds_for_query = 2 * ProtocolState_t::FAILURES_TOLERATED + 4 * log2n + 2
                + (int) ceil(log2(num_meters / (double)(ProtocolState_t::FAILURES_TOLERATED + 1)));
    }
    query_timeout_timer = timer_library.register_timer(rounds_for_query * NETWORK_ROUNDTRIP_TIMEOUT,
            std::bind(&UtilityClient::end_query, this));
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
}

} /* namespace psm */
