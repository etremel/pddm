/*
 * Event.cpp
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#include "Event.h"

namespace pddm {
namespace simulation {

bool Event::operator >(const Event& other) const {
    return other < *this;
}

bool Event::operator <(const Event& other) const {
    return other.fire_time < fire_time;
}

bool Event::operator ==(const Event& other) const {
    return action == other.action && fire_time == other.fire_time && cancelled == other.cancelled && timeout == other.timeout;
}

void Event::fire() {
    if(!cancelled)
        action();
}

} /* namespace simulation */
} /* namespace psm */

