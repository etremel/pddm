/*
 * AggregationMessage.cpp
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#include <cassert>
#include <algorithm>
#include <functional>

#include "AggregationMessage.h"

namespace pddm {
namespace messaging {

//Voodoo incantations because C++ is too stupid to understand static constants
const constexpr MessageType AggregationMessage::type;
const constexpr MessageBodyType AggregationMessageValue::type;

std::ostream& operator<<(std::ostream& out, const AggregationMessageValue& v) {
  if ( !v.empty() ) {
    out << '[';
    std::copy (v.begin(), v.end(), std::ostream_iterator<FixedPoint_t>(out, ", "));
    out << "\b\b]";
  }
  return out;
}

bool operator==(const AggregationMessage& lhs, const AggregationMessage& rhs) {
    return lhs.num_contributors == rhs.num_contributors && lhs.query_num == rhs.query_num && (*lhs.body) == (*rhs.body);
}

bool operator!=(const AggregationMessage& lhs, const AggregationMessage& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& out, const AggregationMessage& m) {
    return out << *m.get_body() << " | Contributors: " << m.get_num_contributors();
}

void AggregationMessage::add_value(const pddm::FixedPoint_t& value, int num_contributors) {
    (*get_body())[0] += value;
    this->num_contributors += num_contributors;
}

void AggregationMessage::add_values(const std::vector<FixedPoint_t>& values, const int num_contributors) {
    assert(values.size() == get_body()->size());
    //Add all elements of values to their corresponding elements in the message body
    std::transform(values.begin(), values.end(), get_body()->begin(), get_body()->begin(), std::plus<FixedPoint_t>());
    this->num_contributors += num_contributors;
}

std::size_t AggregationMessage::bytes_size() const {
    return mutils::bytes_size(type) +
            mutils::bytes_size(num_contributors) +
            mutils::bytes_size(query_num) +
            Message::bytes_size();
}

std::size_t AggregationMessage::to_bytes(char* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(type, buffer);
    bytes_written += mutils::to_bytes(num_contributors, buffer + bytes_written);
    bytes_written += mutils::to_bytes(query_num, buffer + bytes_written);
    bytes_written += Message::to_bytes(buffer + bytes_written);
    return bytes_written;
}

void AggregationMessage::post_object(const std::function<void(const char* const, std::size_t)>& function) const {
    mutils::post_object(function, type);
    mutils::post_object(function, num_contributors);
    mutils::post_object(function, query_num);
    Message::post_object(function);
}

std::unique_ptr<AggregationMessage> AggregationMessage::from_bytes(mutils::DeserializationManager<>* m, const char* buffer) {
    std::size_t bytes_read = 0;
    MessageType message_type;
    std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
    bytes_read += sizeof(MessageType);

    int num_contributors;
    std::memcpy(&num_contributors, buffer + bytes_read, sizeof(num_contributors));
    bytes_read += sizeof(num_contributors);
    int query_num;
    std::memcpy(&query_num, buffer + bytes_read, sizeof(query_num));
    bytes_read += sizeof(query_num);

    //Superclass deserialization
    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(sender_id));
    bytes_read += sizeof(sender_id);
    std::unique_ptr<body_type> body = mutils::from_bytes<body_type>(m, buffer + bytes_read);
    auto body_shared = std::shared_ptr<body_type>(std::move(body));
    return std::unique_ptr<AggregationMessage>(new AggregationMessage(sender_id, query_num, body_shared, num_contributors));
}

} /* namespace messaging */
} /* namespace psm */

