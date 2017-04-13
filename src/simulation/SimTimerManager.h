/*
 * EventTimerManager.h
 *
 *  Created on: May 19, 2016
 *      Author: edward
 */

#pragma once

#include <map>
#include <memory>
#include <functional>

#include "Event.h"
#include "EventManager.h"
#include "../util/TimerManager.h"

namespace pddm {
class UtilityClient;
class MeterClient;
}

namespace pddm {
namespace simulation {

class SimTimerManager: public util::TimerManager {
    private:
        util::timer_id_t next_id;
        std::map<util::timer_id_t, std::weak_ptr<Event>> timer_events;
        EventManager& event_manager;

    public:
        SimTimerManager(EventManager& event_manager) : next_id(0), timer_events(), event_manager(event_manager) {}
        virtual ~SimTimerManager() = default;
        util::timer_id_t register_timer(const int delay_ms, std::function<void(void)> callback) override;
        void cancel_timer(const util::timer_id_t timer_id) override;
};


std::function<SimTimerManager (MeterClient&)> timer_manager_builder(EventManager& event_manager);
std::function<SimTimerManager (UtilityClient&)> timer_manager_builder_utility(EventManager& event_manager);

} /* namespace simulation */
} /* namespace psm */

