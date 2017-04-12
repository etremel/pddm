/*
 * AgreementValue.h
 *
 *  Created on: May 26, 2016
 *      Author: edward
 */

#pragma once

#include <ostream>

#include "MessageBody.h"
#include "SignedValue.h"
#include "../util/CryptoLibrary.h"

namespace pddm {

namespace messaging {

struct AgreementValue : public MessageBody {
        static const constexpr MessageBodyType type = MessageBodyType::AGREEMENT_VALUE;
        SignedValue signed_value;
        /** The ID of the node that signed the SignedValue (after accepting it). */
        int accepter_id;
        /** The signature over the entire SignedValue of a node that accepted the value. */
        util::SignatureArray accepter_signature;

        AgreementValue(const SignedValue& signed_value, const int accepter_id) :
            signed_value(signed_value), accepter_id(accepter_id) {}
        //Member-by-member constructor used only by serialization
        AgreementValue(const SignedValue& signed_value, const int accepter_id, const util::SignatureArray& signature) :
            signed_value(signed_value), accepter_id(accepter_id), accepter_signature(signature) {}
        virtual ~AgreementValue() = default;
        inline bool operator==(const MessageBody& _rhs) const {
            if (auto* rhs = dynamic_cast<const AgreementValue*>(&_rhs))
                return this->signed_value == rhs->signed_value
                        && this->accepter_id == rhs->accepter_id
                        && this->accepter_signature == rhs->accepter_signature;
            else return false;
        }
        std::size_t bytes_size() const;
        std::size_t to_bytes(char* buffer) const;
        void post_object(const std::function<void (char const * const,std::size_t)>&) const;
        static std::unique_ptr<AgreementValue> from_bytes(mutils::DeserializationManager *p, const char* buffer);
};

inline std::ostream& operator<<(std::ostream& out, const AgreementValue val) {
    return out << "{AgreementValue: " << val.signed_value << " accepted by " << val.accepter_id << "}";
}


}  // namespace messaging

}  // namespace pddm


