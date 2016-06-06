/*
 * AggregationMessage.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include "Message.h"
#include "../Configuration.h"

namespace pddm {
namespace messaging {

/** Decorates std::vector<FixedPoint_t> with the MessageBody type so it can be the payload of a Message. */
class AggregationMessageValue : public MessageBody, public std::vector<FixedPoint_t> {};

class AggregationMessage: public Message {
    private:
        int num_contributors;
    public:
        int query_num;
        AggregationMessage() = default;
        AggregationMessage(const int sender_id, const int query_num, std::shared_ptr<AggregationMessageValue> value) :
            Message(sender_id, value), query_num(query_num), num_contributors(1) {}
        std::shared_ptr<AggregationMessageValue> get_body() { return body; };
        void add_value(const FixedPoint_t& value, int num_contributors);
        void add_values(std::vector<FixedPoint_t>& values, const int num_contributors);
        int get_num_contributors() const { return num_contributors; }

        friend bool operator==(const AggregationMessage& lhs, const AggregationMessage& rhs);
        friend struct std::hash<AggregationMessage>;
};

inline bool operator==(const AggregationMessage& lhs, const AggregationMessage& rhs) {
    return lhs.num_contributors == rhs.num_contributors && lhs.query_num == rhs.query_num && (*lhs.body) == (*rhs.body);
}

inline bool operator!=(const AggregationMessage& lhs, const AggregationMessage& rhs) {
    return !(lhs == rhs);
}

} /* namespace messaging */
} /* namespace psm */

namespace std {

template<>
struct hash<pddm::messaging::AggregationMessage> {
        size_t operator()(const pddm::messaging::AggregationMessage& input) const {
            const int prime = 31;
            size_t result = 1;
            result = prime * result + hash(input.num_contributors);
            result = prime * result + hash(input.query_num);
            result = prime * result + hash(*input.body);
            return result;
        }
};

} // namespace std

