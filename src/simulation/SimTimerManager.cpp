/*
 * EventTimerManager.cpp
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#include <string>

#include "SimTimerManager.h"

#include "../MeterClient.h"
#include "../UtilityClient.h"

namespace pddm {
namespace simulation {

using util::timer_id_t;

timer_id_t SimTimerManager::register_timer(const int delay_ms, std::function<void(void)> callback) {
    std::weak_ptr<Event> event_ptr = event_manager.submit(callback, event_manager.get_current_time() + delay_ms, "Timer delayed by " + std::to_string(delay_ms) + " ms", true);
    timer_events[next_id] = event_ptr;
    return next_id++; //Return current value, then increment it for next time
}

void SimTimerManager::cancel_timer(const timer_id_t timer_id) {
    auto entry = timer_events.find(timer_id);
    if(entry != timer_events.end() && !entry->second.expired()) {
        entry->second.lock()->cancel();
        timer_events.erase(entry);
    }
}

std::function<SimTimerManager (MeterClient&)> timer_manager_builder(EventManager& event_manager) {
    return [&event_manager](MeterClient& client) {
        return SimTimerManager(event_manager);
    };
}

std::function<SimTimerManager (UtilityClient&)>timer_manager_builder_utility(EventManager& event_manager) {
    return [&event_manager](UtilityClient& client) {
        return SimTimerManager(event_manager);
    };
}

} /* namespace simulation */
} /* namespace psm */
