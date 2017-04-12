/*
 * PingMessage.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <cstddef>

#include "Message.h"
#include "MessageType.h"

namespace pddm {
namespace messaging {

class PingMessage: public Message {
    public:
        static const constexpr MessageType type = MessageType::PING;
        bool is_response;
        PingMessage(const int sender_id, const bool is_response = false) :
            Message(sender_id, nullptr), is_response(is_response) {}
        virtual ~PingMessage() = default;

        /**
         * Computes the number of bytes it would take to serialize this message.
         * @return The size of a serialized PingMessage in bytes.
         */
        std::size_t bytes_size() const;
        /**
         * Copies a PingMessage into the byte buffer that {@code buffer} points to,
         * blindly assuming that the buffer is large enough to contain the message.
         * The caller must ensure that the buffer is at least as long as
         * message.bytes_size() before calling this.
         * @param buffer The byte buffer into which this object should be serialized.
         */
        std::size_t to_bytes(char* buffer) const;
        void post_object(const std::function<void (char const * const,std::size_t)>& f) const;
        void ensure_registered(mutils::DeserializationManager&){}
        /**
         * Creates a new PingMessage by deserializing the contents of {@code buffer},
         * blindly assuming that the buffer is large enough to contain a PingMessage.
         * @param buffer A byte buffer containing the results of an earlier call to
         * to_bytes(char*).
         * @return A new PingMessage reconstructed from the serialized bytes.
         */
        static std::unique_ptr<PingMessage> from_bytes(mutils::DeserializationManager* m, const char* buffer) ;
};


} /* namespace messaging */
} /* namespace psm */

