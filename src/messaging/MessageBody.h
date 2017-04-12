#pragma once

#include <mutils-serialization/SerializationSupport.hpp>

namespace pddm {

namespace messaging {

/**
 * Marker type asserting that the subtype can be the body of a message.
 * This also helps ensure that all message bodies implement ByteRepresentable
 * for easy serialization.
 */
class MessageBody : public mutils::ByteRepresentable {
    public:
        virtual ~MessageBody() = default;
        virtual bool operator==(const MessageBody&) const = 0;

        void ensure_registered(mutils::DeserializationManager&) override {}

        static std::unique_ptr<MessageBody> from_bytes(mutils::DeserializationManager* m, char const * buffer);
};

inline bool operator!=(const MessageBody& a, const MessageBody& b){
    return !(a == b);
}

} /* namespace messaging */
} /* namespace pddm */



