/**
 * @file OverlayTransportMessage.cpp
 *
 * @date Oct 14, 2016
 * @author edward
 */

#include "OverlayTransportMessage.h"

#include <cstring>
#include <cstddef>
#include <memory>
#include <mutils-serialization/SerializationSupport.hpp>

#include "MessageType.h"
#include "MessageBodyType.h"

namespace pddm {
namespace messaging {

constexpr const MessageType OverlayTransportMessage::type;

std::size_t OverlayTransportMessage::bytes_size() const {
    return mutils::bytes_size(type) +
            mutils::bytes_size(sender_round) +
            mutils::bytes_size(is_final_message) +
            Message::bytes_size();
}

std::size_t OverlayTransportMessage::to_bytes(char* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(type, buffer);
    bytes_written += mutils::to_bytes(sender_round, buffer + bytes_written);
    bytes_written += mutils::to_bytes(is_final_message, buffer + bytes_written);
    bytes_written += Message::to_bytes(buffer + bytes_written);
    return bytes_written;
}


void OverlayTransportMessage::post_object(const std::function<void(const char* const, std::size_t)>& function) const {
    mutils::post_object(function, type);
    mutils::post_object(function, sender_round);
    mutils::post_object(function, is_final_message);
    Message::post_object(function);
}


std::unique_ptr<OverlayTransportMessage> OverlayTransportMessage::from_bytes(mutils::DeserializationManager* m, const char* buffer) {
    std::size_t bytes_read = 0;
    MessageType message_type;
    std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
    bytes_read += sizeof(MessageType);

    int sender_round;
    std::memcpy(&sender_round, buffer + bytes_read, sizeof(sender_round));
    bytes_read += sizeof(sender_round);

    bool is_final_message;
    std::memcpy(&is_final_message, buffer + bytes_read, sizeof(is_final_message));
    bytes_read += sizeof(is_final_message);

    //Superclass deserialization
    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(sender_id));
    bytes_read += sizeof(sender_id);
    std::unique_ptr<body_type> body = mutils::from_bytes<body_type>(m, buffer + bytes_read);
    auto body_shared = std::shared_ptr<body_type>(std::move(body));
    return std::make_unique<OverlayTransportMessage>(sender_id, sender_round, is_final_message, body_shared);
}


}  // namespace messaging
}  // namespace pddm
