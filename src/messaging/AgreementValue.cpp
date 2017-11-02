/**
 * @file AgreementValue.cpp
 *
 * @date Nov 15, 2016
 * @author edward
 */

#include "AgreementValue.h"

namespace pddm {
namespace messaging {

const constexpr MessageBodyType AgreementValue::type;

std::size_t AgreementValue::bytes_size() const {
    //Don't add sizeof(MessageBodyType) because SignedValue already adds it
    return mutils::bytes_size(signed_value) +
            mutils::bytes_size(accepter_id) +
            (accepter_signature.size() * sizeof(util::SignatureArray::value_type));
}

std::size_t AgreementValue::to_bytes(char* buffer) const {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(signed_value, buffer);
    //Rewrite the first two bytes of the buffer to change the MessageBodyType
    mutils::to_bytes(type, buffer);
    //Now add the AgreementValue fields
    bytes_written += mutils::to_bytes(accepter_id, buffer + bytes_written);
    std::memcpy(buffer + bytes_written, accepter_signature.data(),
            accepter_signature.size() * sizeof(util::SignatureArray::value_type));
    bytes_written += accepter_signature.size() * sizeof(util::SignatureArray::value_type);
    return bytes_written;
}

void AgreementValue::post_object(const std::function<void (char const * const,std::size_t)>& consumer) const {
    //This avoids needing to rewrite MessageBodyType after caling post_object(signed_value)
    char buffer[bytes_size()];
    to_bytes(buffer);
    consumer(buffer, bytes_size());
}

std::unique_ptr<AgreementValue> AgreementValue::from_bytes(mutils::DeserializationManager<>* p, const char* buffer) {
    std::unique_ptr<SignedValue> signedval = mutils::from_bytes<SignedValue>(p, buffer);
    std::size_t bytes_read = mutils::bytes_size(*signedval);
    int accepter_id;
    std::memcpy((char*) &accepter_id, buffer + bytes_read, sizeof(accepter_id));
    bytes_read += sizeof(accepter_id);

    util::SignatureArray signature;
    std::memcpy(signature.data(), buffer + bytes_read, signature.size() * sizeof(util::SignatureArray::value_type));
    bytes_read += signature.size() * sizeof(util::SignatureArray::value_type);

    //Ugh, unnecessary copy of the SignedValue. Maybe I should store it by unique_ptr instead of by value.
    return std::make_unique<AgreementValue>(*signedval, accepter_id, signature);
}

}
}


