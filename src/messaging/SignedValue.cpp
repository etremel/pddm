/**
 * @file SignedValue.cpp
 *
 * @date Nov 8, 2016
 * @author edward
 */

#include "SignedValue.h"

#include <cstring>
#include <mutils-serialization/SerializationSupport.hpp>

namespace pddm {

namespace messaging {

//Voodoo incantations
const constexpr MessageBodyType SignedValue::type;

//I don't have time to make this generic for all std::maps
std::size_t SignedValue::bytes_size(const std::map<int, util::SignatureArray>& sig_map) {
    return sizeof(int) + sig_map.size() * (sizeof(int) +
            (util::RSA_SIGNATURE_SIZE * sizeof(util::SignatureArray::value_type)));
}

std::size_t SignedValue::to_bytes(const std::map<int, util::SignatureArray>& sig_map, char * buffer) {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(sig_map.size(), buffer);
    for(const auto& entry : sig_map) {
        bytes_written += mutils::to_bytes(entry.first, buffer + bytes_written);
        std::memcpy(buffer + bytes_written, entry.second.data(), entry.second.size() * sizeof(util::SignatureArray::value_type));
        bytes_written += entry.second.size() * sizeof(util::SignatureArray::value_type);
    }
    return bytes_written;
}

std::unique_ptr<std::map<int, util::SignatureArray>> SignedValue::from_bytes_map(mutils::DeserializationManager<>* p, const char* buffer) {
    std::size_t bytes_read = 0;
    int size;
    std::memcpy((char*) &size, buffer, sizeof(size));
    const char* buf2 = buffer + sizeof(size);
    auto new_map = std::make_unique<std::map<int, util::SignatureArray>>();
    for(int i = 0; i < size; ++i) {
        int key;
        util::SignatureArray value;
        std::memcpy((char*) &key, buf2 + bytes_read, sizeof(key));
        bytes_read += sizeof(key);
        std::memcpy(value.data(), buf2 + bytes_read, value.size() * sizeof(util::SignatureArray::value_type));
        bytes_read += value.size() * sizeof(util::SignatureArray::value_type);
        new_map->emplace(key, value);
    }
    return new_map;
}

std::size_t SignedValue::bytes_size() const {
    //Don't add a sizeof(MessageBodyType) because ValueContribution already adds one
    return mutils::bytes_size(*value) + bytes_size(signatures);
}

std::size_t SignedValue::to_bytes(char* buffer) const {
    //Since *value is itself a MessageBody, this will also put a MessageBodyType in the buffer
    std::size_t bytes_written = mutils::to_bytes(*value, buffer);
    //Rewrite the first two bytes of the buffer to change the MessageBodyType
    mutils::to_bytes(type, buffer);
    //Now append the signatures
    bytes_written += to_bytes(signatures, buffer + bytes_written);
    return bytes_written;
}

void SignedValue::post_object(const std::function<void(const char* const, std::size_t)>& function) const {
    char buffer[bytes_size()];
    to_bytes(buffer);
    function(buffer, bytes_size());
}

std::unique_ptr<SignedValue> SignedValue::from_bytes(mutils::DeserializationManager<>* p, const char* buffer) {
    //Read the ValueContribution, which will also read past the MessageBodyType
    std::unique_ptr<ValueContribution> contribution = mutils::from_bytes<ValueContribution>(p, buffer);
    std::size_t bytes_read = mutils::bytes_size(*contribution);
    std::unique_ptr<std::map<int, util::SignatureArray>> signatures = from_bytes_map(p, buffer + bytes_read);
    //Fix the pointer type of the ValueContribution
    std::shared_ptr<ValueContribution> contribution_shared(std::move(contribution));
    return std::make_unique<SignedValue>(contribution_shared, *signatures);
}

}
}

