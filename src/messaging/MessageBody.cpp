/**
 * @file MessageBody.cpp
 *
 * @date Nov 15, 2016
 * @author edward
 */

#include "MessageBody.h"

#include <memory>

#include "AggregationMessage.h"
#include "AgreementValue.h"
#include "MessageBodyType.h"
#include "PathOverlayMessage.h"
#include "SignedValue.h"
#include "StringBody.h"
#include "ValueContribution.h"

namespace pddm {
namespace messaging {

std::unique_ptr<MessageBody> MessageBody::from_bytes(mutils::DeserializationManager* m, char const * buffer) {
            //Reinterpret the first sizeof(MessageType) bytes of buffer, rather than copying it to a new MessageType
            MessageBodyType body_type = *((MessageBodyType*)(buffer));
            //Dispatch to the correct subclass from_bytes based on the type
            switch(body_type) {
            case OverlayMessage::type:
                return OverlayMessage::from_bytes(m, buffer);
            case PathOverlayMessage::type:
                return PathOverlayMessage::from_bytes(m, buffer);
            case AggregationMessageValue::type:
                return AggregationMessageValue::from_bytes(m, buffer);
            case ValueContribution::type:
                return ValueContribution::from_bytes(m, buffer);
            case SignedValue::type:
                return SignedValue::from_bytes(m, buffer);
            case AgreementValue::type:
                return AgreementValue::from_bytes(m, buffer);
            case StringBody::type:
                return StringBody::from_bytes(m, buffer);
            default:
                return nullptr;
            }
        }

}
}
