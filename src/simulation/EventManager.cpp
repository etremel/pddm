/*
 * EventManager.cpp
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#include "EventManager.h"

#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <memory>

namespace pddm {
namespace simulation {

using std::make_shared;
using std::shared_ptr;
using std::weak_ptr;

void EventManager::run_simulation() {
    bool done = false;
        while (!done) {
            if (!timeout_queue.empty() && simulation_time - last_gc_time > GARBAGE_COLLECT_INTERVAL) {
                garbage_collect_timeouts();
                last_gc_time = simulation_time;
            }
            shared_ptr<Event> next;
            if(!timeout_queue.empty() &&
                    (event_queue.empty() || timeout_queue.top() < event_queue.top())) {
                next = std::move(timeout_queue.top());
                timeout_queue.pop();
            } else if (!event_queue.empty() &&
                    (timeout_queue.empty() || event_queue.top() < timeout_queue.top())) {
                next = std::move(event_queue.top());
                event_queue.pop();
            }

            if(next == nullptr) { //Both queues were empty
                done = true;
            } else {
                if(next->get_fire_time() < simulation_time) {
                    throw std::runtime_error("Error! Simulation time moved backwards!");
                }
                simulation_time = next->get_fire_time();
                next->fire();
            }
            //The Event should get cleaned up now, as soon as the local pointer goes out of scope
            assert(next.unique());
        }
}

/**
 * This constructs a new Event inside the simulator, using the supplied
 * arguments as the Event's constructor arguments. The returned pointer is a
 * weak_ptr to the created event, which is owned by the EventManager and will
 * be deleted after it fires.
 * @param action The action to run when the event fires.
 * @param fire_time The time, in milliseconds, at which the event should fire
 * @param is_timeout Whether the event should be marked as a "timeout" event,
 *        which means it is likely to be cancelled before it fires. Defaults
 *        to false.
 * @return A weak_ptr to the Event that was created and submitted by this method.
 */
std::weak_ptr<Event> EventManager::submit(const Event::Action& action, const long long fire_time, const bool is_timeout) {
    if(fire_time < simulation_time) {
        throw std::runtime_error("Attempted to submit an event in the past!");
    }
    shared_ptr<Event> event = make_shared<Event>(action, fire_time, is_timeout);
    weak_ptr<Event> event_ptr(event);
    if(is_timeout) {
        //Avoid an unnecessary copy of the pointer; I'd really rather emplace()
        timeout_queue.push(std::move(event));
    } else {
        event_queue.push(std::move(event));
    }
    return event_ptr;
}

void EventManager::garbage_collect_timeouts() {
    event_queue_t filtered_queue;
    while(!timeout_queue.empty()) {
        const shared_ptr<Event>& event = timeout_queue.top();
        if(!event->is_cancelled()) {
            filtered_queue.push(event);
        }
        timeout_queue.pop();
    }
    std::swap(timeout_queue, filtered_queue);
}

void EventManager::reset() {
    simulation_time = 0;
    //Clear the event queues
    event_queue_t empty1;
    event_queue_t empty2;
    std::swap(event_queue, empty1);
    std::swap(timeout_queue, empty2);
}

} /* namespace simulation */
} /* namespace psm */

