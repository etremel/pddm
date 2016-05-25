#pragma once

namespace pddm {

namespace messaging {

/**
 * Marker type asserting that the subtype can be the body of a messsage.
 * Necessary because C++ doesn't have "Object" as a common supertype of everything.
 */
class MessageBody {
    public:
        virtual ~MessageBody() = 0;
};

}
}



