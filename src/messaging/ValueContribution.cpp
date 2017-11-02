/**
 * @file ValueContribution.cpp
 *
 * @date Nov 7, 2016
 * @author edward
 */

#include "ValueContribution.h"

#include <mutils-serialization/SerializationSupport.hpp>

namespace pddm {
namespace messaging {

const constexpr MessageBodyType ValueContribution::type;

std::size_t ValueContribution::to_bytes(char* buffer) const {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(type, buffer);
    //Unpack each member of value and write it individually, since ValueTuple isn't ByteSerializable
    bytes_written += mutils::to_bytes(value.query_num, buffer + bytes_written);
    bytes_written += mutils::to_bytes(value.value, buffer + bytes_written);
    bytes_written += mutils::to_bytes(value.proxies, buffer + bytes_written);
    std::memcpy(buffer + bytes_written, signature.data(), signature.size() * sizeof(util::SignatureArray::value_type));
    bytes_written += signature.size() * sizeof(util::SignatureArray::value_type);
    return bytes_written;
}

void ValueContribution::post_object(const std::function<void(const char* const, std::size_t)>& consumer_function) const {
    mutils::post_object(consumer_function, type);
    mutils::post_object(consumer_function, value.query_num);
    mutils::post_object(consumer_function, value.value);
    mutils::post_object(consumer_function, value.proxies);
    consumer_function((const char*) signature.data(), signature.size() * sizeof(util::SignatureArray::value_type));
}

std::size_t ValueContribution::bytes_size() const {
    return mutils::bytes_size(type) + mutils::bytes_size(value.query_num) +
            mutils::bytes_size(value.value) + mutils::bytes_size(value.proxies) +
            (signature.size() * sizeof(util::SignatureArray::value_type));
}

std::unique_ptr<ValueContribution> ValueContribution::from_bytes(mutils::DeserializationManager<>* m, const char* buffer) {
    std::size_t bytes_read = 0;
    MessageBodyType type;
    std::memcpy(&type, buffer + bytes_read, sizeof(type));
    bytes_read += sizeof(type);

    int query_num;
    std::memcpy(&query_num, buffer + bytes_read, sizeof(query_num));
    bytes_read += sizeof(query_num);
    std::unique_ptr<std::vector<FixedPoint_t>> value_vector =
            mutils::from_bytes<std::vector<FixedPoint_t>>(nullptr, buffer + bytes_read);
    bytes_read += mutils::bytes_size(*value_vector);
    std::unique_ptr<std::vector<int>> proxy_vector =
            mutils::from_bytes<std::vector<int>>(nullptr, buffer + bytes_read);
    bytes_read += mutils::bytes_size(*proxy_vector);

    util::SignatureArray signature;
    std::memcpy(signature.data(), buffer + bytes_read, signature.size() * sizeof(util::SignatureArray::value_type));
    bytes_read += signature.size() * sizeof(util::SignatureArray::value_type);

    //This will unnecessarily copy the vectors into the new ValueTuple, but
    //it's the only thing we can do because from_bytes returns unique_ptr
    return std::make_unique<ValueContribution>(ValueTuple{query_num, *value_vector, *proxy_vector}, signature);
}

}
}

