/*
 * Device.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include <vector>

#include "../FixedPoint_t.h"

namespace pddm {

namespace simulation {

/**
 * A simple data object holding the characteristics of a simulated electricity-using
 * device. Collections of devices are used to generate simulated electrical
 * consumption for a simulation::Meter.
 */
struct Device {
    /** The device's name, as used in the configuration files; must uniquely identify the device */
    std::string name;
    /** in Watts; array size is number of possible unique cycles */
    std::vector<FixedPoint_t> load_per_cycle;
    /**in minutes; should align with loadPerCycle array */
    std::vector<int> time_per_cycle;
    /** in Watts */
    FixedPoint_t standby_load;
    double weekday_frequency; //mean daily starting frequency
    double weekend_frequency;
    /** hour-by-hour usage probability for weekdays */
    std::vector<double> weekday_hourly_probability;
    /** hour-by-hour usage probability for weekends */
    std::vector<double> weekend_hourly_probability;
    /** if the customer will turn off this device when they can't afford the energy prices */
    bool disable_to_save_money;

    Device() : name(""), standby_load(0.0), weekday_frequency(0), weekend_frequency(0), disable_to_save_money(false) {};
};

}
}


