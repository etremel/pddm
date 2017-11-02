#pragma once

#include <mutils-serialization/SerializationSupport.hpp>

namespace pddm {

namespace messaging {

/**
 * Superclass for all types that can be the body of a message. Ensures that all
 * message bodies are subtypes of ByteRepresentable, and provides an "interface"
 * version of from_bytes (not actually virtual) that manually implements dynamic
 * dispatch to the correct subclass's version of from_bytes.
 */
class MessageBody : public mutils::ByteRepresentable {
    public:
        virtual ~MessageBody() = default;
        virtual bool operator==(const MessageBody&) const = 0;

        static std::unique_ptr<MessageBody> from_bytes(mutils::DeserializationManager<>* m, char const * buffer);
};

inline bool operator!=(const MessageBody& a, const MessageBody& b){
    return !(a == b);
}

} /* namespace messaging */
} /* namespace pddm */



