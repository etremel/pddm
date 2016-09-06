/*
 * SignatureResponse.h
 *
 *  Created on: May 23, 2016
 *      Author: edward
 */

#pragma once

#include <memory>

#include "Message.h"
#include "StringBody.h"

namespace pddm {
namespace messaging {

class SignatureResponse: public Message {
    public:
        using body_type = StringBody;
        SignatureResponse(const int sender_id, const std::shared_ptr<StringBody>& encrypted_response) :
            Message(sender_id, encrypted_response) {};
};

} /* namespace messaging */
} /* namespace psm */

