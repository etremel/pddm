/*
 * SignatureRequest.h
 *
 *  Created on: May 20, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <string>

#include "Message.h"
#include "MessageType.h"
#include "StringBody.h"

namespace pddm {
namespace messaging {

class SignatureRequest : public Message {
    public:
        static const constexpr MessageType type = MessageType::SIGNATURE_REQUEST;
        using body_type = StringBody;
        SignatureRequest(const int sender_id, const std::shared_ptr<StringBody>& encrypted_value) :
            Message(sender_id, encrypted_value) {}
        virtual ~SignatureRequest() = default;

        //Trivial extension of Message, just call superclass's serialization methods
        std::size_t bytes_size() const {
            return mutils::bytes_size(type) + Message::bytes_size();
        }
        std::size_t to_bytes(char* buffer) const {
            std::size_t bytes_written = mutils::to_bytes(type, buffer);
            bytes_written += Message::to_bytes(buffer + bytes_written);
            return bytes_written;
        }
        void post_object(const std::function<void(const char* const, std::size_t)>& function) const {
            mutils::post_object(function, type);
            Message::post_object(function);
        }
        static std::unique_ptr<SignatureRequest> from_bytes(mutils::DeserializationManager<>* m, char const * buffer) {
            std::size_t bytes_read = 0;
            MessageType message_type;
            std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
            bytes_read += sizeof(MessageType);

            int sender_id;
            std::memcpy(&sender_id, buffer + bytes_read, sizeof(sender_id));
            bytes_read += sizeof(sender_id);
            std::unique_ptr<body_type> body = mutils::from_bytes<body_type>(m, buffer + bytes_read);
            //Whack the pointer type into the one SignatureRequest expects
            auto body_shared = std::shared_ptr<body_type>(std::move(body));
            return std::make_unique<SignatureRequest>(sender_id, body_shared);
        }

};

}
}


