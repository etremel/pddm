/**
 * @file Message.cpp
 *
 * @date Nov 15, 2016
 * @author edward
 */


#include "Message.h"

#include "AggregationMessage.h"
#include "MessageType.h"
#include "OverlayTransportMessage.h"
#include "PingMessage.h"
#include "QueryRequest.h"
#include "SignatureRequest.h"
#include "SignatureResponse.h"

namespace pddm {
namespace messaging {

std::size_t Message::bytes_size() const {
     return mutils::bytes_size(sender_id) + mutils::bytes_size(*body);
}

std::size_t Message::to_bytes(char* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(sender_id, buffer);
    //body is not optional, so there's no need to check if it's null
    //(the only subclass that doesn't use body also overrides this method)
    bytes_written += mutils::to_bytes(*body, buffer + bytes_written);
    return bytes_written;
}

void Message::post_object(const std::function<void(const char* const, std::size_t)>& function) const {
    mutils::post_object(function, sender_id);
    mutils::post_object(function, *body);
}

std::unique_ptr<Message> Message::from_bytes(mutils::DeserializationManager* m, char const * buffer) {
    //Peek at the first sizeof(MessageType) bytes of buffer to determine the MessageType
    MessageType message_type = *((MessageType*)(buffer));
    //Dispatch to the correct subclass from_bytes based on the type
    switch(message_type) {
    case OverlayTransportMessage::type:
        return OverlayTransportMessage::from_bytes(m, buffer);
    case PingMessage::type:
        return PingMessage::from_bytes(m, buffer);
    case AggregationMessage::type:
        return AggregationMessage::from_bytes(m, buffer);
    case QueryRequest::type:
        return QueryRequest::from_bytes(m, buffer);
    case SignatureRequest::type:
        return SignatureRequest::from_bytes(m, buffer);
    case SignatureResponse::type:
        return SignatureResponse::from_bytes(m, buffer);
    default:
        return nullptr;
    }
}

}
}

