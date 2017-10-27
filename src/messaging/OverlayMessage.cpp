/**
 * @file OverlayMessage.cpp
 *
 * @date Sep 12, 2016
 * @author edward
 */

#include <ostream>
#include <memory>
#include <cstring>
#include <mutils-serialization/SerializationSupport.hpp>

#include "OverlayMessage.h"

#include "AgreementValue.h"
#include "PathOverlayMessage.h"
#include "QueryRequest.h"
#include "SignedValue.h"
#include "StringBody.h"
#include "ValueContribution.h"

namespace pddm {
namespace messaging {

const constexpr MessageBodyType OverlayMessage::type;

std::ostream& operator<< (std::ostream& out, const OverlayMessage& message) {
    out << "{QueryNum=" << message.query_num << "|Destination=" << message.destination << "|Body=" ;
    //Force C++ to use dynamic dispatch on operator<< even though it doesn't want to
    if(message.body == nullptr) {
        out << "null";
    } else if(auto av_body = std::dynamic_pointer_cast<AgreementValue>(message.body)) {
        out << *av_body;
    } else if(auto pom_body = std::dynamic_pointer_cast<PathOverlayMessage>(message.body)) {
        out << *pom_body;
    } else if(auto om_body = std::dynamic_pointer_cast<OverlayMessage>(message.body)) {
        out << *om_body;
    } else if(auto sv_body = std::dynamic_pointer_cast<SignedValue>(message.body)) {
        out << *sv_body;
    } else if(auto string_body = std::dynamic_pointer_cast<StringBody>(message.body)) {
        out << *string_body;
    } else if(auto vc_body = std::dynamic_pointer_cast<ValueContribution>(message.body)) {
        out << *vc_body;
    } else {
        out << "UNKNOWN TYPE @ " << message.body;
    }
    out << "}";
    return out;
}

std::size_t OverlayMessage::bytes_size() const {
    return mutils::bytes_size(type)
            + mutils::bytes_size(query_num) + mutils::bytes_size(destination)
            + mutils::bytes_size(is_encrypted) + mutils::bytes_size(flood)
            + mutils::bytes_size(false) //Represents the "remaining_body" variable
            + (body == nullptr ? 0 : mutils::bytes_size(*body));
}

//Is it OK to implement post_object this way?? Or do I have to recursively call post_object,
//which would require another _common method for the superclass?
void OverlayMessage::post_object(const std::function<void(const char* const, std::size_t)>& consumer_function) const {
    char buffer[bytes_size()];
    to_bytes(buffer);
    consumer_function(buffer, bytes_size());
}

std::size_t OverlayMessage::to_bytes_common(char* buffer) const {
    std::size_t bytes_written = 0;
    //For POD types, this is just a shortcut to memcpy(buffer+bytes_written, &var, sizeof(var))
    bytes_written += mutils::to_bytes(query_num, buffer + bytes_written);
    bytes_written += mutils::to_bytes(destination, buffer + bytes_written);
    bytes_written += mutils::to_bytes(is_encrypted, buffer + bytes_written);
    bytes_written += mutils::to_bytes(flood, buffer + bytes_written);

    bool remaining_body = (body != nullptr);
    bytes_written += mutils::to_bytes(remaining_body, buffer + bytes_written);
    if(remaining_body) {
        bytes_written += mutils::to_bytes(*body, buffer + bytes_written);
    }
    return bytes_written;
}

std::size_t OverlayMessage::to_bytes(char* buffer) const {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(type, buffer);
    bytes_written += to_bytes_common(buffer + bytes_written);
    return bytes_written;
}

std::unique_ptr<OverlayMessage> OverlayMessage::from_bytes(mutils::DeserializationManager* p, char const * buffer) {
    std::size_t bytes_read = 0;
    MessageBodyType type;
    std::memcpy(&type, buffer + bytes_read, sizeof(type));
    bytes_read += sizeof(type);

    //We can't use the private constructor with make_unique
    auto constructed_message = std::unique_ptr<OverlayMessage>(new OverlayMessage());
    bytes_read += from_bytes_common(*constructed_message, buffer + bytes_read);
    return std::move(constructed_message);
}

std::size_t OverlayMessage::from_bytes_common(OverlayMessage& partial_overlay_message, char const * buffer) {
    std::size_t bytes_read = 0;
    std::memcpy(&partial_overlay_message.query_num, buffer + bytes_read, sizeof(partial_overlay_message.query_num));
    bytes_read += sizeof(partial_overlay_message.query_num);
    std::memcpy(&partial_overlay_message.destination, buffer + bytes_read, sizeof(partial_overlay_message.destination));
    bytes_read += sizeof(partial_overlay_message.destination);
    std::memcpy(&partial_overlay_message.is_encrypted, buffer + bytes_read, sizeof(partial_overlay_message.is_encrypted));
    bytes_read += sizeof(partial_overlay_message.is_encrypted);
    std::memcpy(&partial_overlay_message.flood, buffer + bytes_read, sizeof(partial_overlay_message.flood));
    bytes_read += sizeof(partial_overlay_message.flood);

    bool remaining_body;
    std::memcpy(&remaining_body, buffer + bytes_read, sizeof(remaining_body));
    bytes_read += sizeof(remaining_body);
    if(remaining_body) {
        partial_overlay_message.body = mutils::from_bytes<MessageBody>(nullptr, buffer+bytes_read);
        bytes_read += mutils::bytes_size(*partial_overlay_message.body);
    }

    return bytes_read;
}

} /* namespace messaging */
} /* namespace pddm */

