#pragma once

#include <memory>
#include <mutils-serialization/SerializationSupport.hpp>

#include "MessageBody.h"

namespace pddm {

/** The "node ID" that will be used to refer to the utility/system owner in
 *  messages. The utility is not a node that participates in the overlay, but
 *  message-passing functions still need to send messages to the utility. */
const int UTILITY_NODE_ID = -1;

namespace messaging {

/**
 * Base class for all messages sent between devices in this system.
 * Defines the header fields that are common to all messages.
 */
class Message : public mutils::ByteRepresentable {
    public:
        int sender_id;
        std::shared_ptr<MessageBody> body;
        Message(const int sender_id, std::shared_ptr<MessageBody> body) : sender_id(sender_id), body(std::move(body)) {};
        virtual ~Message() = default;
        //Common serialization code for the superclass header fields.
        //These functions DO NOT include a MessageType field, which must be added by a subclass.
        std::size_t bytes_size() const;
        std::size_t to_bytes(char* buffer) const;
        void post_object(const std::function<void (char const * const,std::size_t)>&) const;
        void ensure_registered(mutils::DeserializationManager&) {}

        //Calls a subclass from_bytes
        static std::unique_ptr<Message> from_bytes(mutils::DeserializationManager* m, char const * buffer);
};

}

}

