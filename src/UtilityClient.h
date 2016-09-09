#pragma once

#include <memory>
#include <unordered_set>
#include <list>

#include "Configuration.h"
#include "messaging/AggregationMessage.h"
#include "messaging/SignatureRequest.h"
#include "messaging/QueryRequest.h"
#include "util/PointerUtil.h"
#include "ConfigurationIncludes.h"
#include "spdlog/spdlog.h"

namespace pddm {

/**
 * The client that runs at the utility to handle sending out query requests and
 * receiving responses.
 */
class UtilityClient {
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
        int query_timeout_timer;
        int query_num;
        bool query_finished;
        util::unordered_ptr_multiset<messaging::AggregationMessage> query_results;
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
    public:
        UtilityClient(const int num_meters, const std::function<UtilityNetworkClient_t (UtilityClient&)>& network_builder,
                const std::function<CryptoLibrary_t (UtilityClient&)>& crypto_library_builder,
                const std::function<TimerManager_t (UtilityClient&)>& timer_library_builder) :
                    logger(spdlog::get("global_logger")), num_meters(num_meters), network(network_builder(*this)),
                    crypto_library(crypto_library_builder(*this)), timer_library(timer_library_builder(*this)),
                    query_timeout_timer(0), query_num(-1), query_finished(false) {}
        /** Handles receiving an AggregationMessage from a meter, which should contain a query result. */
        void handle_message(const std::shared_ptr<messaging::AggregationMessage>& message);

        /** Handles receiving a SignatureRequest from a meter, by signing the requested value. */
        void handle_message(const std::shared_ptr<messaging::SignatureRequest>& message);

        /** Starts a query by broadcasting a message from the utility to all the meters in the network.*/
        void start_query(const std::shared_ptr<messaging::QueryRequest>& query);

        /** Starts a batch of queries that should be executed in sequence as quickly as possible */
        void start_queries(const std::list<std::shared_ptr<messaging::QueryRequest>>& queries);

        /** The maximum time (ms) the utility is willing to wait on a network round-trip */
        static constexpr int NETWORK_ROUNDTRIP_TIMEOUT = 100;
    private:
        void end_query();

};

} /* namespace psm */

