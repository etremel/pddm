/*
 * AgreementValue.h
 *
 *  Created on: May 26, 2016
 *      Author: edward
 */

#pragma once
#include "MessageBody.h"
#include "SignedValue.h"
#include "../util/CryptoLibrary.h"

namespace pddm {

namespace messaging {

struct AgreementValue : public MessageBody {
        SignedValue signed_value;
        int accepter_id;
        util::SignatureArray accepter_signature;
};

}  // namespace messaging

}  // namespace pddm


