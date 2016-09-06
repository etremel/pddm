/*
 * ValueTuple.h
 *
 *  Created on: May 20, 2016
 *      Author: edward
 */

#pragma once

#include "MessageBody.h"
#include "../util/Hash.h"
#include "../FixedPoint_t.h"

namespace pddm {

namespace messaging {

struct ValueTuple : public MessageBody {
        int query_num;
        std::vector<FixedPoint_t> value;
        std::vector<int> proxies;
        //Member-by-member constructor should do the obvious thing
        ValueTuple(const int query_num, const std::vector<FixedPoint_t>& value, const std::vector<int>& proxies) :
            query_num(query_num), value(value), proxies(proxies) {}
//        ValueTuple(const ValueTuple&) = default;
//        ValueTuple(ValueTuple&&) = default;
        //Equality operator should do the obvious thing.
        inline bool operator==(const MessageBody& _rhs) const {
            if (auto* rhs = dynamic_cast<const ValueTuple*>(&_rhs))
                return this->query_num == rhs->query_num
                        && this->value == rhs->value
                        && this->proxies == rhs->proxies;
            else return false;
        }
};

}  // namespace messaging

} // namespace pddm

namespace std {

template<>
struct hash<pddm::messaging::ValueTuple> {
        size_t operator()(const pddm::messaging::ValueTuple& input) const {
            using pddm::util::hash_combine;
            size_t result = 1;
            hash_combine(result, input.query_num);
            hash_combine(result, input.value);
            hash_combine(result, input.proxies);
            return result;
        }
};

} // namespace std



