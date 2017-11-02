/**
 * @file PingMessage.cpp
 *
 * @date Oct 13, 2016
 * @author edward
 */

#include "PingMessage.h"

#include <cstddef>
#include <cstring>
#include <cassert>

namespace pddm {
namespace messaging {

const constexpr MessageType PingMessage::type;

std::size_t PingMessage::bytes_size() const {
    return mutils::bytes_size(type) +
            mutils::bytes_size(sender_id) +
            mutils::bytes_size(is_response);
}

//This completely overrides Message::to_bytes, since PingMessage ignores the body field
std::size_t PingMessage::to_bytes(char* buffer) const {
    std::size_t bytes_written = 0;
    std::memcpy(buffer+bytes_written, &type, sizeof(MessageType));
    bytes_written += sizeof(MessageType);
    std::memcpy(buffer+bytes_written, &sender_id, sizeof(sender_id));
    bytes_written += sizeof(sender_id);
    std::memcpy(buffer+bytes_written, &is_response, sizeof(is_response));
    bytes_written += sizeof(is_response);
    return bytes_written;
}

void PingMessage::post_object(const std::function<void(const char*, std::size_t)>& consumer) const {
    mutils::post_object(consumer, type);
    mutils::post_object(consumer, sender_id);
    mutils::post_object(consumer, is_response);
}

std::unique_ptr<PingMessage> PingMessage::from_bytes(mutils::DeserializationManager<>* m, const char* buffer) {
    std::size_t bytes_read = 0;
    MessageType message_type;
    std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
    bytes_read += sizeof(MessageType);
    assert(message_type == MessageType::PING);
    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(int));
    bytes_read += sizeof(int);
    bool is_response;
    std::memcpy(&is_response, buffer + bytes_read, sizeof(bool));
    bytes_read += sizeof(bool);
    return std::make_unique<PingMessage>(sender_id, is_response);
}


std::ostream& operator<< (std::ostream& out, const PingMessage& message) {
    return out << "Ping from " << message.sender_id << ", is_response = " << std::boolalpha << message.is_response;
}

}
}
