/*
 * SignedValue.h
 *
 *  Created on: May 25, 2016
 *      Author: edward
 */

#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "ValueContribution.h"
#include "../util/CryptoLibrary.h"

namespace pddm {
namespace messaging {

struct SignedValue : public MessageBody {
        std::shared_ptr<ValueContribution> value;
        /** Maps the meter ID of a meter to that meter's signature on this
         * message's ValueContribution. */
        std::map<int, util::SignatureArray> signatures;

        inline bool operator==(const MessageBody& _rhs) const {
            if (auto* rhs = dynamic_cast<const SignedValue*>(&_rhs))
                return (rhs->value == nullptr ? value == rhs->value : *value == *(rhs->value))
                        && this->signatures == rhs->signatures;
            else return false;
        }
};

}
}


