/*
 * ValueTuple.h
 *
 *  Created on: May 20, 2016
 *      Author: edward
 */

#pragma once

namespace pddm {

namespace messaging {

struct ValueTuple : public MessageBody {
        int query_num;
        std::vector<FixedPoint_t> value;
        std::vector<int> proxies;
};

//Equality operator should do the obvious thing.
inline bool operator==(const ValueTuple& lhs, const ValueTuple& rhs) {
    return lhs.query_num == rhs.query_num
            && lhs.value == rhs.value
            && lhs.proxies == rhs.proxies;
}

inline bool operator!=(const ValueTuple& lhs, const ValueTuple& rhs) {
    return !(lhs == rhs);
}

}  // namespace messaging

} // namespace pddm

namespace std {

template<>
struct hash<pddm::messaging::ValueTuple> {
        size_t operator()(const pddm::messaging::ValueTuple& input) const {
            const int prime = 31;
            size_t result = 1;
            result = prime * result + hash(input.query_num);
            result = prime * result + hash(input.value);
            result = prime * result + hash(input.proxies);
            return result;
        }
};

} // namespace std



