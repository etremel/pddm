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
#include <ostream>

#include "ValueTuple.h"
#include "MessageBody.h"
#include "MessageBodyType.h"
#include "../util/CryptoLibrary.h"
#include "../util/Hash.h"

namespace pddm {

namespace messaging {

/**
 * Represents a signed (round, value, proxies) tuple that can be contributed to
 * an aggregation query.
 */
struct ValueContribution : public MessageBody {
        static const constexpr MessageBodyType type = MessageBodyType::VALUE_CONTRIBUTION;
        ValueTuple value;
        util::SignatureArray signature;
        ValueContribution(const ValueTuple& value) : value(value) {
            signature.fill(0);
        }
        ValueContribution(const ValueTuple& value, const util::SignatureArray& signature) :
            value(value), signature(signature) {}
        virtual ~ValueContribution() = default;

        inline bool operator==(const MessageBody& _rhs) const {
            if (auto* rhs = dynamic_cast<const ValueContribution*>(&_rhs))
                return this->value == rhs->value && this->signature == rhs->signature;
            else return false;
        }

        //Serialization support
        std::size_t to_bytes(char* buffer) const override;
        void post_object(const std::function<void (char const * const,std::size_t)>& consumer_function) const override;
        std::size_t bytes_size() const override;
        static std::unique_ptr<ValueContribution> from_bytes(mutils::DeserializationManager* m, char const * buffer);

};

inline std::ostream& operator<<(std::ostream& stream, const ValueContribution& vc) {
    return stream << "{ValueContribution: " << vc.value << "}";
}

}  // namespace messaging

}  // namespace pddm


namespace std {

template<>
struct hash<pddm::messaging::ValueContribution> {
        size_t operator()(const pddm::messaging::ValueContribution& input) const {
            size_t result = 1;
            pddm::util::hash_combine(result, input.signature);
            pddm::util::hash_combine(result, input.value);
            return result;
        }
};

} // namespace std
