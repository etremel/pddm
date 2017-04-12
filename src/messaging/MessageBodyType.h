/**
 * @file MessageBodyType.h
 *
 * @date Oct 14, 2016
 * @author edward
 */

#pragma once

#include <cstdint>

namespace pddm {
namespace messaging {

enum class MessageBodyType : int16_t {
    OVERLAY, /* Always refers to an OverlayMessage, not an OverlayTransportMessage */
    PATH_OVERLAY,
    AGREEMENT_VALUE,
    SIGNED_VALUE,
    VALUE_CONTRIBUTION,
    AGGREGATION_VALUE,
    STRING
};

}
}

