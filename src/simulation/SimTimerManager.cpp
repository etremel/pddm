/*
 * EventTimerManager.cpp
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#include "SimTimerManager.h"


namespace pddm {
namespace simulation {

int SimTimerManager::register_timer(const int delay_ms, std::function<void(void)> callback) {
    std::weak_ptr<Event> event_ptr = event_manager.submit(callback, event_manager.simulation_time + delay_ms, true);
    timer_events[next_id] = event_ptr;
    return next_id++; //Return current value, then increment it for next time
}

void SimTimerManager::cancel_timer(const int timer_id) {
    auto entry = timer_events.find(timer_id);
    if(entry != timer_events.end() && !entry->second.expired()) {
        entry->second.lock()->cancel();
        timer_events.erase(entry);
    }
}

TimerManagerBuilderFunc timer_manager_builder(const EventManager& event_manager) {
    return [&event_manager](MeterClient& client) {
        return SimTimerManager(event_manager);
    };
}

std::function<TimerManager_t (UtilityClient&)> timer_manager_builder_utility(const EventManager& event_manager) {
    return [&event_manager](UtilityClient& client) {
        return SimTimerManager(event_manager);
    };
}

} /* namespace simulation */
} /* namespace psm */
