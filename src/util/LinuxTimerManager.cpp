/**
 * @file LinuxTimerManager.cpp
 *
 * @date Oct 7, 2016
 * @author edward
 */

#include "LinuxTimerManager.h"

#include <cassert>
#include <csignal>
#include <ctime>
#include <cstring>

namespace pddm {
namespace util {

//Ugly hack. There better only be once instance of LinuxTimerManager, otherwise the signal handler won't work.
LinuxTimerManager* tm_instance;

LinuxTimerManager::LinuxTimerManager() : next_id(0) {
    tm_instance = this;
    //Register timer_signal_handler as the signal handler for timers
    struct sigaction sa = {0};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_signal_handler;
    sigemptyset(&sa.sa_mask);
    int success_flag = sigaction(SIGRTMIN, &sa, NULL);
    assert(success_flag == 0);
}

/* This function needs access to the list of callbacks, but that's stored
 * in the LinuxTimerManager object, and this must be a bare function. The
 * only solution I can see is to use a global pointer to one instance of
 * LinuxTimerManager. */
void timer_signal_handler(int signum, siginfo_t* info, void* ucontext) {
    timer_id_t timer_id = info->si_value.sival_int;
    //Only fire the trigger if the timer is not in the cancelled set
    auto cancelled_location = tm_instance->cancelled_timers.find(timer_id);
    if(cancelled_location == tm_instance->cancelled_timers.end()) {
        tm_instance->timer_callbacks.at(timer_id)();
    } else {
        tm_instance->cancelled_timers.erase(cancelled_location);
    }
    tm_instance->timer_callbacks.erase(timer_id);
    timer_delete(tm_instance->timer_handles.at(timer_id));
    tm_instance->timer_handles.erase(timer_id);
}

timer_id_t LinuxTimerManager::register_timer(const int delay_ms, std::function<void(void)> callback) {
    //Create a timer that will signal with a sigev_value containing its ID
    struct sigevent timer_event = {0};
    timer_event.sigev_notify = SIGEV_SIGNAL;
    timer_event.sigev_signo = SIGRTMIN;
    timer_event.sigev_value.sival_int = next_id;

    timer_t timer_handle;
    int success_flag = timer_create(CLOCK_REALTIME, &timer_event, &timer_handle);
    assert(success_flag == 0);
    timer_handles[next_id] = timer_handle;
    timer_callbacks[next_id] = callback;

    //Arm the timer to fire once after delay_ms milliseconds
    struct itimerspec timer_spec = {0};
    timer_spec.it_interval.tv_sec = 0;
    timer_spec.it_interval.tv_nsec = 0;
    timer_spec.it_value.tv_sec = 0;
    timer_spec.it_value.tv_nsec = delay_ms * 1000000;
    timer_settime(timer_handle, 0, &timer_spec, NULL);

    return next_id++; //Return current value, then increment it for next time

}

/*
 * Mark the timer as cancelled rather than attempt to delete it, which might
 * create a race condition with the timer handler firing.
 */
void LinuxTimerManager::cancel_timer(const timer_id_t timer_id) {
    if(timer_handles.find(timer_id) != timer_handles.end()) {
        cancelled_timers.emplace(timer_id);
    }
}

std::function<LinuxTimerManager(MeterClient&)> timer_manager_builder() {
    return [](MeterClient& client) {
        return LinuxTimerManager();
    };
}

std::function<LinuxTimerManager(UtilityClient&)> timer_manager_builder_utility() {
    return [](UtilityClient& client) {
        return LinuxTimerManager();
    };
}

} /* namespace util */
} /* namespace pddm */

