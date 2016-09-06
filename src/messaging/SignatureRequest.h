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
#include "StringBody.h"

namespace pddm {
namespace messaging {

class SignatureRequest : public Message {
    public:
        using body_type = StringBody;
        SignatureRequest(const int sender_id, const std::shared_ptr<StringBody>& encrypted_value) :
            Message(sender_id, encrypted_value) {}
};

}
}


