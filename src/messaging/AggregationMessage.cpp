/*
 * AggregationMessage.cpp
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#include <cassert>
#include <algorithm>
#include <functional>

#include "AggregationMessage.h"

namespace pddm {
namespace messaging {


void AggregationMessage::add_value(const pddm::FixedPoint_t& value, int num_contributors) {
    (*get_body())[0] += value;
    this->num_contributors += num_contributors;
}

void AggregationMessage::add_values(const std::vector<FixedPoint_t>& values, const int num_contributors) {
    assert(values.size() == get_body()->size());
    //Add all elements of values to their corresponding elements in the message body
    std::transform(values.begin(), values.end(), get_body()->begin(), get_body()->begin(), std::plus<FixedPoint_t>());
    this->num_contributors += num_contributors;
}

} /* namespace messaging */
} /* namespace psm */
