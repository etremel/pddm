/*
 * OverlayMessage.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <ostream>

#include "MessageBody.h"
#include "MessageBodyType.h"
#include "../util/Hash.h"

namespace pddm {
namespace messaging {

/**
 * This is the payload of an OverlayTransportMessage, which may contain as its body
 * another OverlayMessage if the message is an encrypted onion.
 */
class OverlayMessage: public MessageBody {
    public:
        static const constexpr MessageBodyType type = MessageBodyType::OVERLAY;
        int query_num;
        int destination;
        bool is_encrypted; //In the simulation, this will be a marker for whether we should pretend this message is encrypted
        bool flood; //True if this message should be sent out on every round, regardless of destination
        std::shared_ptr<MessageBody> body;
        OverlayMessage(const int query_num, const int dest_id, const std::shared_ptr<MessageBody>& body, const bool flood = false) :
                query_num(query_num), destination(dest_id), is_encrypted(false), flood(flood), body(body) {}
        virtual ~OverlayMessage() = default;

        inline bool operator==(const MessageBody& _rhs) const {
            auto lhs = this;
            if (auto* rhs = dynamic_cast<const OverlayMessage*>(&_rhs))
                return lhs->query_num == rhs->query_num
                        && lhs->destination == rhs->destination
                        && lhs->is_encrypted == rhs->is_encrypted
                        && lhs->flood == rhs->flood
                        && (lhs->body == nullptr ? rhs->body == nullptr :
                                (rhs->body != nullptr && *lhs->body == *rhs->body));
            else return false;
        }
        /**
         * Computes the number of bytes it would take to serialize this message.
         * @return The size of this message when serialized, in bytes.
         */
        std::size_t bytes_size() const;

        /**
         * Copies an OverlayMessage into the byte buffer that {@code buffer}
         * points to, blindly assuming that the buffer is large enough to contain the
         * message. The caller must ensure that the buffer is at least as long as
         * bytes_size(message) before calling this.
         * @param buffer The byte buffer into which it should be serialized.
         * @return The number of bytes written; should be equal to bytes_size()
         */
        std::size_t to_bytes(char* buffer) const;

        void post_object(const std::function<void (char const * const,std::size_t)>& consumer_function) const;


        /**
         * Creates a new OverlayMessage by deserializing the contents of
         * {@code buffer}, blindly assuming that the buffer is large enough to contain
         * the OverlayMessage and its enclosed body (if present).
         * @param buffer A byte buffer containing the results of an earlier call to
         * OverlayMessage::to_bytes(char*).
         * @return A new OverlayMessage reconstructed from the serialized bytes.
         */
        static std::unique_ptr<OverlayMessage> from_bytes(mutils::DeserializationManager* p, const char * buffer);

    protected:
        /** Default constructor, used only when reconstructing serialized messages */
        OverlayMessage() : query_num(0), destination(0), is_encrypted(false), flood(false), body(nullptr) {}
        /**
         * Helper method for implementing to_bytes; serializes the superclass
         * fields (from OverlayMessage) into the given buffer. Subclasses can
         * call this to get OverlayMessage's implementation of to_bytes
         * *without* getting a MessageBodyType inserted into the buffer.
         * @param buffer The byte buffer into which OverlayMessage fields should
         * be serialized.
         * @return The number of bytes written by this method.
         */
        std::size_t to_bytes_common(char* buffer) const;

        /**
         * Helper method for implementing from_bytes; deserializes the superclass
         * (OverlayMessage) fields from buffer into the corresponding fields of
         * the provided partially-constructed OverlayMessage object. Assumes
         * that the pointer to the buffer is already offset to the correct
         * position at which to start reading OverlayMessage fields. Subclasses
         * can call this to get OverlayMessgae's implementation of from_bytes
         * without the overhead of creating an extra copy of the message.
         * @param partial_overlay_message An OverlayMessage whose fields should
         * be updated to contain the values in the buffer
         * @param buffer The byte buffer containing serialized OverlayMessage fields
         * @return The number of bytes read from the buffer during deserialization.
         */
        static std::size_t from_bytes_common(OverlayMessage& partial_overlay_message, const char * buffer);


};

std::ostream& operator<< (std::ostream& out, const OverlayMessage& message);


} /* namespace messaging */
} /* namespace psm */


namespace std {

template<>
struct hash<pddm::messaging::OverlayMessage> {
        size_t operator()(const pddm::messaging::OverlayMessage& input) const {
            using pddm::util::hash_combine;

            size_t result = 1;
            hash_combine(result, input.query_num);
            hash_combine(result, input.destination);
            hash_combine(result, input.is_encrypted);
            hash_combine(result, input.flood);
            hash_combine(result, input.body);
            return result;
        }
};

}  // namespace std
