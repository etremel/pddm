/*
 * PingMessage.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include "Message.h"

namespace pddm {
namespace messaging {

class PingMessage: public Message {
    public:
        bool is_response;
        PingMessage(const int sender_id, const bool is_response = false) :
            Message(sender_id, nullptr), is_response(is_response) {}
        virtual ~PingMessage() = default;
};

} /* namespace messaging */
} /* namespace psm */

