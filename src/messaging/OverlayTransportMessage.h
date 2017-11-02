/*
 * OverlayTransportMessage.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <ostream>
#include <cstddef>

#include "Message.h"
#include "MessageType.h"
#include "OverlayMessage.h"

namespace pddm {
namespace messaging {

/**
 * The type of message that is sent between nodes during any round of the
 * peer-to-peer overlay. It contains the header fields that change every time a
 * message is sent, and "wraps" a body message that may be relayed unchanged
 * over several rounds of messaging.
 */
class OverlayTransportMessage: public Message {
    public:
        static const constexpr MessageType type = MessageType::OVERLAY;
        /** An OverlayTransportMessage may contain an OverlayMessage or a
         *  subclass of OverlayMessage, but OverlayMessage will at least be
         *  a valid cast target. */
        using body_type = OverlayMessage;
        int sender_round;
        bool is_final_message;
        OverlayTransportMessage(const int sender_id, const int sender_round,
                const bool is_final_message, std::shared_ptr<OverlayMessage> wrapped_message) :
            Message(sender_id, wrapped_message), sender_round(sender_round), is_final_message(is_final_message) {};
        virtual ~OverlayTransportMessage() = default;

        std::size_t bytes_size() const;
        std::size_t to_bytes(char* buffer) const;
        void post_object(const std::function<void (char const * const,std::size_t)>&) const;
        static std::unique_ptr<OverlayTransportMessage> from_bytes(mutils::DeserializationManager<>* m, char const * buffer);
};

std::ostream& operator<< (std::ostream& out, const OverlayTransportMessage& message);


} /* namespace messaging */
} /* namespace psm */
