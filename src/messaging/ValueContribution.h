/*
 * ValueContribution.h
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#pragma once

#include <vector>
#include <tuple>
#include <cstdint>

#include "ValueTuple.h"
#include "../util/CryptoLibrary.h"

namespace pddm {

namespace messaging {

/**
 * Represents a signed (round, value, proxies) tuple that can be contributed to
 * an aggregation query.
 */
struct ValueContribution {
        ValueTuple value;
        uint8_t signature[util::RSA_SIGNATURE_SIZE];
};

}  // namespace messaging

}  // namespace psm


