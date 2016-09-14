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
#include "../util/Hash.h"

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
    OverlayMessage(const int query_num, const int dest_id, const std::shared_ptr<MessageBody>& body, bool flood = false) :
        query_num(query_num), destination(dest_id), is_encrypted(false), flood(flood), body(body) {};
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
