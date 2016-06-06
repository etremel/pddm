/*
 * ValueContribution.h
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#pragma once

#include <vector>
#include <tuple>
#include <cstdint>
#include <cstring>

#include "ValueTuple.h"
#include "../util/CryptoLibrary.h"

namespace pddm {

namespace messaging {

/**
 * Represents a signed (round, value, proxies) tuple that can be contributed to
 * an aggregation query.
 */
struct ValueContribution : public MessageBody {
        ValueTuple value;
        util::SignatureArray signature;
};

inline bool operator==(const ValueContribution& lhs, const ValueContribution& rhs) {
    return lhs.value == rhs.value && memcmp(lhs.signature, rhs.signature, util::RSA_SIGNATURE_SIZE) == 0;
}

inline bool operator!=(const ValueContribution& lhs, const ValueContribution& rhs) {
    return !(lhs == rhs);
}

}  // namespace messaging

}  // namespace psm


namespace std {

template<>
struct hash<pddm::messaging::ValueContribution> {
        size_t operator()(const pddm::messaging::ValueContribution& input) const {
            const int prime = 31;
            size_t result = 1;
            result = prime * result + hash(input.signature);
            result = prime * result + hash(input.value);
            return result;
        }
};

} // namespace std
