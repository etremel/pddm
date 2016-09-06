#pragma once

namespace pddm {

namespace messaging {

/**
 * Marker type asserting that the subtype can be the body of a messsage.
 * Necessary because C++ doesn't have "Object" as a common supertype of everything.
 */
class MessageBody {
    protected:
        virtual ~MessageBody() = default;
    public:
        virtual bool operator==(const MessageBody&) const = 0;
};

inline bool operator!=(const MessageBody& a, const MessageBody& b){
    return !(a == b);
}

} /* namespace messaging */
} /* namespace pddm */



