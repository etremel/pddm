/*
 * SimParameters.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

/**
 * @file SimParameters.h
 * Contains assorted "magic numbers" for the simulation of private smart metering.
 */

namespace pddm {
namespace simulation {

const int SIMULATION_DAYS = 1;
/** The number of minutes represented by one usage timestep. To prevent rounding errors,
 * this should be a number that divides 1440 (the number of minutes in a day) */
const int USAGE_TIMESTEP_MIN = 10;

constexpr int TOTAL_TIMESTEPS = 7;

const int PERCENT_POOR_HOMES = 25;
const int PERCENT_RICH_HOMES = 25;

const int METER_FAILURES_PER_QUERY = 0;

//Duration time assumptions for cryptography operations
const int RSA_DECRYPT_TIME_MICROS = 461;
const int RSA_ENCRYPT_TIME_MICROS = 30;
const int RSA_SIGN_TIME_MICROS = 458;
const int RSA_VERIFY_TIME_MICROS = 27;

}
}


