/*
 * TimerManager.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include <functional>

namespace pddm {
namespace util {

using timer_id_t = int;

/**
 * Interface for a class that can create timers. Timers may either be implemented
 * as actual timer interrupts, or as simulation events if the MeterClient is running
 * in a simulation.
 */
class TimerManager {
    public:
        /**
         * Creates a new timer that will invoke the specified function after a
         * delay.
         * @param delay_ms The time to wait for, in milliseconds
         * @param callback The function that will be invoked when the timer expires
         * @return A unique identifier for this timer, which will allow it to
         * be cancelled later.
         */
        virtual timer_id_t register_timer(const int delay_ms, std::function<void(void)> callback) = 0;
        /**
         * Cancels a timer that has not yet expired, so that it will take no
         * action when it expires. If this is called on a timer that has already
         * expired, it does nothing.
         * @param timer_id The unique identifier for the timer to cancel.
         */
        virtual void cancel_timer(const timer_id_t timer_id) = 0;
        virtual ~TimerManager() = 0;
};

inline TimerManager::~TimerManager() { }

}
}


