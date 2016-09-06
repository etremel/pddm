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
    bool flood; //True if this message should be sent out on every round, regardless of destination
    std::shared_ptr<MessageBody> body;
    OverlayMessage(const int query_num, const int dest_id, const std::shared_ptr<MessageBody>& body, bool is_encrypted = false, bool flood = false) :
        query_num(query_num), destination(dest_id), is_encrypted(is_encrypted), flood(flood), body(body) {};
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

};

} /* namespace messaging */
} /* namespace psm */
