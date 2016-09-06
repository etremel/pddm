/*
 * OverlayTransportMessage.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include <memory>

#include "Message.h"
#include "OverlayMessage.h"

namespace pddm {
namespace messaging {

class OverlayTransportMessage: public Message {
    public:
        using body_type = OverlayMessage;
        int sender_round;
        bool is_final_message;
        OverlayTransportMessage(const int sender_id, const int sender_round,
                const bool is_final_message, std::shared_ptr<OverlayMessage> wrapped_message) :
            Message(sender_id, wrapped_message), sender_round(sender_round), is_final_message(is_final_message) {};
};

} /* namespace messaging */
} /* namespace psm */
