/*
 * DeviceState.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

namespace pddm {

namespace simulation {

struct DeviceState {
        /**time the device was actually started today*/
        int start_time;
        /** time the device is scheduled to start, if it is shiftable and being delayed */
        int scheduled_start_time;
        bool is_on;
        int current_cycle_num;
        /**Number of minutes into the current cycle, if a timestep occurred in the middle of a cycle */
        int time_in_current_cycle;
};

}

}


