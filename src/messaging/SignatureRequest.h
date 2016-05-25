/*
 * SignatureRequest.h
 *
 *  Created on: May 20, 2016
 *      Author: edward
 */

#pragma once

#include <memory>
#include <string>

#include "Message.h"
#include "MessageBody.h"

namespace pddm {
namespace messaging {

/** Decorates std::string with the MessageBody type so it can be the payload of a Message. */
class StringBody : public MessageBody, public std::string {};

class SignatureRequest : public Message {
    public:
        SignatureRequest(const int sender_id, const std::shared_ptr<StringBody>& encrypted_value) :
            Message(sender_id, encrypted_value) {}
};

}
}


