/*
 * SignedValue.h
 *
 *  Created on: May 25, 2016
 *      Author: edward
 */

#pragma once

#include <cstdint>
#include <map>
#include "ValueContribution.h"
#include "../util/CryptoLibrary.h"

namespace pddm {
namespace messaging {

struct SignedValue : public MessageBody {
        ValueContribution value;
        std::map<int, util::SignatureArray> signatures;
};

}
}


