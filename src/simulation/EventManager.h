/*
 * EventManager.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <queue>
#include <functional>
#include <vector>
#include <memory>

#include "Event.h"
#include "../util/PointerUtil.h"

namespace pddm {
namespace simulation {

class EventManager {
    private:
        long long simulation_time;
        //Alias because the typename for "A priority queue of Events" is absurdly long
        using event_queue_t = std::priority_queue<std::shared_ptr<Event>, std::vector<std::shared_ptr<Event>>, util::ptr_greater<Event>>;
        event_queue_t event_queue;
        event_queue_t timeout_queue;
        long long last_gc_time;
        static const int GARBAGE_COLLECT_INTERVAL = 20;
        void garbage_collect_timeouts();
    public:
        /**  Runs the entire simulation to completion; processes events until either
         * there are no more events, or a terminal event is encountered. */
        void run_simulation();
        /** Submits a new event to the simulator and returns a pointer to the created event. */
        std::weak_ptr<Event> submit(const Event::Action& action, const long long fire_time, const bool is_timeout = false);
        long long get_current_time() const { return simulation_time; };
        void reset();
};

} /* namespace simulation */
} /* namespace psm */

