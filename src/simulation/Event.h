/*
 * Event.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <functional>

namespace pddm {
namespace simulation {

/**
 * Represents an event in the simulation. Holds a function pointer representing
 * the action to take, and keeps track of the time it will fire and whether it's
 * been cancelled.
 */
class Event {
    public:
        using Action = std::function<void(void)>;
    private:
        Action action;
        long long fire_time;
        bool cancelled;
        bool timeout;
    public:
        Event(const Action& action, const long long fire_time, const bool is_timeout = false, const bool is_cancelled = false) :
            action(action), fire_time(fire_time), cancelled(is_cancelled), timeout(is_timeout) {}
        Event(Event&) = delete;
        Event(Event&&) = default;

        /** Compares Events by their fire time; a < b if a will fire before b. */
        bool operator>(const Event& other) const;
        /** Compares Events by their fire time; a < b if a will fire before b. */
        bool operator<(const Event& other) const;
//        /** Deep equality comparison. Two events are only equal if they contain the same action and have the same fire time. */
//        bool operator==(const Event& other) const;
        void fire();
        /** Cancels the event. This does not necessarily delete the event,
         * but ensures that it will take no action when fired. */
        void cancel() { cancelled = true; }
        bool is_cancelled() const { return cancelled; }
        /** @return True if this event represents some kind of timeout. */
        bool is_timeout() const { return timeout; }
        /** @return the simulated time at which this event will occur. */
        long long get_fire_time() { return fire_time; }

};

} /* namespace simulation */
} /* namespace psm */
