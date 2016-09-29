#pragma once

#include <memory>
#include <unordered_set>
#include <list>
#include <spdlog/spdlog.h>

#include "Configuration.h"
#include "messaging/AggregationMessage.h"
#include "messaging/SignatureRequest.h"
#include "messaging/QueryRequest.h"
#include "util/PointerUtil.h"
#include "ConfigurationIncludes.h"

namespace pddm {

/**
 * The client that runs at the utility to handle sending out query requests and
 * receiving responses.
 */
class UtilityClient {
    public:
        using QueryCallback = std::function<void (const int, const std::shared_ptr<messaging::AggregationMessageValue>&)>;
    private:
        enum class QueryProtocol { BFT, CT, HFT };
        /* Instead of making three subclasses of UtilityClient, we'll just switch behavior
         * based on which subclass of ProtocolState we're using. This statically initializes
         * the value of query_protocol based on which type ProtocolState_t is mapped to. */
        static constexpr QueryProtocol query_protocol = std::is_same<ProtocolState_t, BftProtocolState>::value ?
                QueryProtocol::BFT :
                (std::is_same<ProtocolState_t, HftProtocolState>::value ?
                        QueryProtocol::HFT : QueryProtocol::CT);
        std::shared_ptr<spdlog::logger> logger;
        const int num_meters;
        UtilityNetworkClient_t network;
        CryptoLibrary_t crypto_library;
        TimerManager_t timer_library;
        /** Number of milliseconds to wait for a query timeout interval */
        const int query_timeout_time;
        /** Handle referring to the timer that was set to time-out the current query*/
        int query_timeout_timer;
        int query_num;
        bool query_finished;
        std::map<int, QueryCallback> query_callbacks;
        util::unordered_ptr_multiset<messaging::AggregationMessage> curr_query_results;
        /** All results of queries the utility has issued, indexed by query number. */
        std::vector<std::shared_ptr<messaging::AggregationMessageValue>> all_query_results;
        std::set<int> curr_query_meters_signed;
        //"A priority queue of pointers to QueryRequests, ordered by QueryNumGreater"
        using query_priority_queue = std::priority_queue<
                std::shared_ptr<messaging::QueryRequest>,
                std::vector<std::shared_ptr<messaging::QueryRequest>>,
                util::ptr_comparator<messaging::QueryRequest, messaging::QueryNumGreater>
        >;
        query_priority_queue pending_batch_queries;
        static int compute_timeout_time(const int num_meters);
    public:
        UtilityClient(const int num_meters, const std::function<UtilityNetworkClient_t (UtilityClient&)>& network_builder,
                const std::function<CryptoLibrary_t (UtilityClient&)>& crypto_library_builder,
                const std::function<TimerManager_t (UtilityClient&)>& timer_library_builder) :
                    logger(spdlog::get("global_logger")),
                    num_meters(num_meters),
                    network(network_builder(*this)),
                    crypto_library(crypto_library_builder(*this)),
                    timer_library(timer_library_builder(*this)),
                    query_timeout_time(compute_timeout_time(num_meters)),
                    query_timeout_timer(0),
                    query_num(-1),
                    query_finished(false) {}
        /** Handles receiving an AggregationMessage from a meter, which should contain a query result. */
        void handle_message(const std::shared_ptr<messaging::AggregationMessage>& message);

        /** Handles receiving a SignatureRequest from a meter, by signing the requested value. */
        void handle_message(const std::shared_ptr<messaging::SignatureRequest>& message);

        /** Starts a query by broadcasting a message from the utility to all the meters in the network.*/
        void start_query(const std::shared_ptr<messaging::QueryRequest>& query);

        /** Starts a batch of queries that should be executed in sequence as quickly as possible */
        void start_queries(const std::list<std::shared_ptr<messaging::QueryRequest>>& queries);

        /** Registers a callback function that should be run each time a query completes. */
        int register_query_callback(const QueryCallback& callback);

        /** Deregisters a callback function previously registered, using its ID. */
        bool deregister_query_callback(const int callback_id);

        /** Gets the stored result of a query that has completed. */
        std::shared_ptr<messaging::AggregationMessageValue> get_query_result(const int query_num) { return all_query_results.at(query_num); }

        /** The maximum time (ms) the utility is willing to wait on a network round-trip */
        static constexpr int NETWORK_ROUNDTRIP_TIMEOUT = 100;
    private:
        void end_query();

};

} /* namespace psm */

