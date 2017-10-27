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
#include "PathOverlayMessage.h"

namespace pddm {
namespace messaging {

constexpr const MessageType OverlayTransportMessage::type;

std::ostream& operator<< (std::ostream& out, const OverlayTransportMessage& message) {
    out << "{SenderRound=" << message.sender_round << "|Final=" << std::boolalpha << message.is_final_message << "|";
    //Force C++ to use dynamic dispatch on operator<< even though it doesn't want to
    if(auto pom_body = std::dynamic_pointer_cast<PathOverlayMessage>(message.body)) {
        out << *pom_body;
    } else if(auto om_body = std::dynamic_pointer_cast<OverlayMessage>(message.body)) {
        out << *om_body;
    } else {
        out << "BODY UNKNOWN TYPE";
    }
    out << "}";
    return out;
}

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

    //Deserialize the fields written by Message's serialization (in the superclass call to to_bytes)
    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(sender_id));
    bytes_read += sizeof(sender_id);
    //Peek ahead at type, but don't advance bytes_read because MessageBody classes will expect to deserialize it
    MessageBodyType type;
    std::memcpy(&type, buffer + bytes_read, sizeof(type));
    //Dispatch to the correct subclass based on the MessageBodyType
    std::shared_ptr<OverlayMessage> body_shared;
    switch(type) {
    case MessageBodyType::OVERLAY: {
        std::unique_ptr<OverlayMessage> body = mutils::from_bytes<OverlayMessage>(m, buffer + bytes_read);
        body_shared = std::shared_ptr<OverlayMessage>(std::move(body));
        break;
    }
    case MessageBodyType::PATH_OVERLAY: {
        std::unique_ptr<PathOverlayMessage> body = mutils::from_bytes<PathOverlayMessage>(m, buffer + bytes_read);
        body_shared = std::shared_ptr<PathOverlayMessage>(std::move(body));
        break;
    }
    default: {
        std::cerr << "OverlayTransportMessage contained something other than an OverlayMessage! type = " << static_cast<int16_t>(type) << std::endl;
        assert(false);
        break;
    }
    }
    return std::make_unique<OverlayTransportMessage>(sender_id, sender_round, is_final_message, body_shared);
}


}  // namespace messaging
}  // namespace pddm
