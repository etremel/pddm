#pragma once

#include <memory>
#include <mutils-serialization/SerializationSupport.hpp>

#include "MessageBody.h"

namespace pddm {

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

