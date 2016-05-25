/*
 * Timesteps.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include "SimParameters.h"

namespace pddm {
namespace simulation {

namespace timesteps {

/** Converts a timestep value into a number of minutes since time 0 */
int minute(const int timestep) {
    return timestep * USAGE_TIMESTEP_MIN;
}

/** Converts a timestep value into a number of hours since time 0 */
int hour(const int timestep) {
    return minute(timestep) / 60; //int division rounds down, which is what we want
}

/** Converts a timestep value into a number of days since time 0 */
int day(const int timestep) {
    return hour(timestep) / 24; //int division rounds down, which is what we want
}

/** Converts a timestep value into a number of milliseconds since time 0 */
long millisecond(const int timestep) {
    return minute(timestep) * 60000;
}

}

}
}


