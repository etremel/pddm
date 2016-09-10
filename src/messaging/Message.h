#pragma once

#include <memory>

#include "MessageBody.h"

namespace pddm {

namespace messaging {

/**
 * Base class for all messages sent between devices in this system.
 * Defines the header fields that are common to all messages.
 */
class Message {
    public:
        int sender_id;
        std::shared_ptr<MessageBody> body;
        Message(const int sender_id, std::shared_ptr<MessageBody> body) : sender_id(sender_id), body(std::move(body)) {};
        virtual ~Message() = default;
};

}

}

