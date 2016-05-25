/*
 * SignatureResponse.h
 *
 *  Created on: May 23, 2016
 *      Author: edward
 */

#pragma once

#include "Message.h"
#include "MessageBody.h"

namespace pddm {
namespace messaging {

class StringBody : public MessageBody, public std::string {};

class SignatureResponse: public Message {
    public:
        SignatureResponse(const int sender_id, const std::shared_ptr<StringBody>& encrypted_response) :
            Message(sender_id, encrypted_response) {};
};

} /* namespace messaging */
} /* namespace psm */

