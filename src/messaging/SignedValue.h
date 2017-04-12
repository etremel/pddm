/*
 * SignedValue.h
 *
 *  Created on: May 25, 2016
 *      Author: edward
 */

#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <utility>

#include "MessageBody.h"
#include "MessageBodyType.h"
#include "../util/CryptoLibrary.h"
#include "ValueContribution.h"

namespace pddm {
namespace messaging {

class SignedValue : public MessageBody {
    public:
        static const constexpr MessageBodyType type = MessageBodyType::SIGNED_VALUE;
        std::shared_ptr<ValueContribution> value;
        /** Maps the meter ID of a meter to that meter's signature on this
         * message's ValueContribution. */
        std::map<int, util::SignatureArray> signatures;
        SignedValue() : value(nullptr) {}
        SignedValue(const std::shared_ptr<ValueContribution>& value,
                const std::map<int, util::SignatureArray>& signatures) :
            value(value), signatures(signatures) {}

        inline bool operator==(const MessageBody& _rhs) const {
            if (auto* rhs = dynamic_cast<const SignedValue*>(&_rhs))
                return (rhs->value == nullptr ? value == rhs->value : *value == *(rhs->value))
                        && this->signatures == rhs->signatures;
            else return false;
        }
        std::size_t bytes_size() const;
        std::size_t to_bytes(char* buffer) const;
        void post_object(const std::function<void (char const * const,std::size_t)>& function) const;
        static std::unique_ptr<SignedValue> from_bytes(mutils::DeserializationManager *p, const char* buffer);
    private:
        //Helper functions for the signatures map
        //I don't have time to make this generic for all std::maps
        static std::size_t bytes_size(const std::map<int, util::SignatureArray>& sig_map);
        static std::size_t to_bytes(const std::map<int, util::SignatureArray>& sig_map, char * buffer);
        static std::unique_ptr<std::map<int, util::SignatureArray>> from_bytes_map(mutils::DeserializationManager* p, const char* buffer);
};

inline std::ostream& operator<<(std::ostream& out, const SignedValue& s) {
    out << "{SignedValue: " << *s.value << "| Signatures from: " << '[';
    for(const auto& id_sig : s.signatures) {
        out << id_sig.first << ", ";
    }
    out << "\b\b]" << "}";
    return out;
}

}
}


