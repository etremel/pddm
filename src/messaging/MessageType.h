/*
 * MessageType.h
 *
 *  Created on: May 20, 2016
 *      Author: edward
 */

#pragma once

#include <cstdint>

namespace pddm {
namespace messaging {

enum class MessageType : int16_t {
    OVERLAY, /* Indicates an OverlayTransportMessage, not an OverlayMessage, since OverlayMessages will always be "wrapped" */
    PING,
    AGGREGATION,
    QUERY_REQUEST,
    SIGNATURE_REQUEST,
    SIGNATURE_RESPONSE
};

}
}


