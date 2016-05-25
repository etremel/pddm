/*
 * PathOverlayMessage.h
 *
 *  Created on: May 23, 2016
 *      Author: edward
 */

#pragma once

#include <list>

#include "OverlayMessage.h"

namespace pddm {
namespace messaging {

/**
 * Represents a non-onion-encrypted OverlayMessage that must traverse a path
 * through the overlay. The destination field should contain the ID of the next
 * destination on the path, and the remaining_path field should contain a list
 * of the IDs to forward to after destination.
 */
class PathOverlayMessage : public OverlayMessage {
    public:
        std::list<int> remaining_path;
        PathOverlayMessage(const int query_num, const std::list<int>& path, const std::shared_ptr<MessageBody>& body) :
            OverlayMessage(query_num, path.front(), body), remaining_path(path) {
            remaining_path.pop_front();
        }
};

}
}


