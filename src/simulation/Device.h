/*
 * Device.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include <vector>

#include "../Configuration.h"

namespace pddm {

namespace simulation {

struct Device {
    std::string name; //must uniquely identify the device
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

    Device(Device&&) = default;
};

}
}


