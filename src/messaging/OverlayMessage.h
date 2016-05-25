/*
 * OverlayMessage.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include "MessageBody.h"

namespace pddm {
namespace messaging {

/**
 * This is the payload of an OverlayTransportMessage, which may contain as its body
 * another OverlayMessage if the message is an encrypted onion.
 */
class OverlayMessage: public MessageBody {
    public:
        int query_num;
        int destination;
        bool is_encrypted; //In the simulation, this will be a marker for whether we should pretend this message is encrypted
        std::shared_ptr<MessageBody> body;
        OverlayMessage(const int query_num, const int dest_id, const std::shared_ptr<MessageBody>& body, bool is_encrypted = false) :
            query_num(query_num), destination(dest_id), body(body), is_encrypted(is_encrypted) {};
};

} /* namespace messaging */
} /* namespace psm */
