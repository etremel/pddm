/**
 * @file OverlayMessage.cpp
 *
 * @date Sep 12, 2016
 * @author edward
 */

#include <ostream>
#include <memory>

#include "OverlayMessage.h"

#include "AgreementValue.h"
#include "PathOverlayMessage.h"
#include "QueryRequest.h"
#include "SignedValue.h"
#include "StringBody.h"
#include "ValueContribution.h"

namespace pddm {
namespace messaging {

std::ostream& operator<< (std::ostream& out, const OverlayMessage& message) {
    out << "{QueryNum=" << message.query_num << "|Destination=" << message.destination << "|Body=" ;
    //Force C++ to use dynamic dispatch on operator<< even though it doesn't want to
    if(auto av_body = std::dynamic_pointer_cast<AgreementValue>(message.body)) {
        out << *av_body;
    } else if(auto pom_body = std::dynamic_pointer_cast<PathOverlayMessage>(message.body)) {
        out << *pom_body;
    } else if(auto om_body = std::dynamic_pointer_cast<OverlayMessage>(message.body)) {
        out << *om_body;
    } else if(auto qr_body = std::dynamic_pointer_cast<QueryRequest>(message.body)) {
        out << *qr_body;
    } else if(auto sv_body = std::dynamic_pointer_cast<SignedValue>(message.body)) {
        out << *sv_body;
    } else if(auto string_body = std::dynamic_pointer_cast<StringBody>(message.body)) {
        out << *string_body;
    } else if(auto vc_body = std::dynamic_pointer_cast<ValueContribution>(message.body)) {
        out << *vc_body;
    }
    out << "}";
    return out;
}

}
}

