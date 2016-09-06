/*
 * QueryRequest.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include <functional>

#include "MessageBody.h"
#include "../util/Money.h"

namespace pddm {
namespace messaging {

enum class QueryType {CURR_USAGE_SUM, CURR_USAGE_HISTOGRAM, AVAILABLE_OFFSET_BREAKDOWN, CUMULATIVE_USAGE, PROJECTED_SUM, PROJECTED_HISTOGRAM};

constexpr bool is_single_valued_query(const QueryType& query_type) {
    return query_type == QueryType::CURR_USAGE_SUM || query_type == QueryType::CUMULATIVE_USAGE || query_type == QueryType::PROJECTED_SUM;
}

class QueryRequest: public MessageBody {
        using PriceFunction = std::function<Money (int)>;
    public:
        const QueryType request_type;
        const int time_window;
        const int query_number;
        const PriceFunction proposed_price_function;
        QueryRequest(const QueryType& request_type, const int time_window, const int query_number, const PriceFunction& proposed_price_function) :
            request_type(request_type), time_window(time_window), query_number(query_number), proposed_price_function(proposed_price_function) {}
        QueryRequest(const QueryType& request_type, const int time_window, const int query_number) :
            request_type(request_type), time_window(time_window), query_number(query_number) {}
        inline bool operator==(const MessageBody& _rhs) const {
            if (auto* rhs = dynamic_cast<const QueryRequest*>(&_rhs))
                return this->request_type == rhs->request_type
                        && this->time_window == rhs->time_window
                        && this->query_number == rhs->query_number;
            else return false;
        }
};

struct QueryNumLess {
        inline bool operator()(const QueryRequest& lhs, const QueryRequest& rhs) const {
            return lhs.query_number < rhs.query_number;
        }
};

struct QueryNumGreater {
        inline bool operator()(const QueryRequest& lhs, const QueryRequest& rhs) const {
            return lhs.query_number > rhs.query_number;
        }
};

} /* namespace messaging */
} /* namespace psm */

