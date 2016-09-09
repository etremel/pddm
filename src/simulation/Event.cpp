/*
 * Event.cpp
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#include "Event.h"

namespace pddm {
namespace simulation {

void Event::fire() {
    if(!cancelled)
        action();
}

std::ostream& operator<<(std::ostream& stream, const Event& e) {
    return stream << "Event at " << e.fire_time << ": " << e.name;
}

} /* namespace simulation */
} /* namespace psm */

