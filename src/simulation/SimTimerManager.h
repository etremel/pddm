/*
 * EventTimerManager.h
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#pragma once

#include <map>
#include <memory>

#include "Event.h"
#include "EventManager.h"

namespace pddm {
namespace simulation {

class SimTimerManager: public util::TimerManager {
    private:
        int next_id;
        std::map<int, std::weak_ptr<Event>> timer_events;
        EventManager& event_manager;

    public:
        SimTimerManager(const EventManager& event_manager) : next_id(0), timer_events(), event_manager(event_manager) {}
        virtual ~SimTimerManager();
        int register_timer(const int delay_ms, std::function<void(void)> callback);
        void cancel_timer(const int timer_id);
};


TimerManagerBuilderFunc timer_manager_builder(const EventManager& event_manager);
std::function<TimerManager_t (UtilityClient&)> timer_manager_builder_utility(const EventManager& event_manager);

} /* namespace simulation */
} /* namespace psm */

