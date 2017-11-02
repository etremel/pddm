/*
 * PathOverlayMessage.h
 *
 *  Created on: May 23, 2016
 *      Author: edward
 */

#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <mutils-serialization/SerializationSupport.hpp>
#include <ostream>

#include "OverlayMessage.h"

namespace pddm {
namespace messaging {

/**
 * Represents a non-onion-encrypted OverlayMessage that must traverse a path
 * through the overlay. The destination field should contain the ID of the next
 * destination on the path, and the remaining_path field should contain a list
 * of the IDs to forward to after destination. This is another possible body of
 * an OverlayTransportMessage.
 */
class PathOverlayMessage : public OverlayMessage {
    public:
        static const constexpr MessageBodyType type = MessageBodyType::PATH_OVERLAY;
        std::list<int> remaining_path;
        PathOverlayMessage(const int query_num, const std::list<int>& path, const std::shared_ptr<MessageBody>& body) :
            OverlayMessage(query_num, path.front(), body), remaining_path(++path.begin(), path.end()) { }
        virtual ~PathOverlayMessage() = default;

        //Serialization support
        std::size_t to_bytes(char* buffer) const;
        void post_object(const std::function<void (char const * const,std::size_t)>& consumer_function) const;
        std::size_t bytes_size() const;
        static std::unique_ptr<PathOverlayMessage> from_bytes(mutils::DeserializationManager<>* m, char const * buffer);

    protected:
        /** Default constructor, used only by deserialization.*/
        PathOverlayMessage() : OverlayMessage() {}
};


std::ostream& operator<< (std::ostream& out, const PathOverlayMessage& message);

}
}


