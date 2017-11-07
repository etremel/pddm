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

LinuxTimerManager::LinuxTimerManager() :
        timer_signal_num(SIGRTMIN),
        next_id(0),
        fired_timers(1000), /* Initialize with plenty of space to avoid memory allocation */
        thread_shutdown(false) {
    tm_instance = this;
    int sem_success = sem_init(&timers_fired_semaphore, 0, 0);
    assert(sem_success == 0);
    //Register timer_signal_handler as the signal handler for timers
    struct sigaction sa = {0};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_signal_handler;
    sigemptyset(&sa.sa_mask);
    int sig_success = sigaction(timer_signal_num, &sa, NULL);
    assert(sig_success == 0);
    callback_thread = std::thread(&LinuxTimerManager::callback_thread_main, this);
}

LinuxTimerManager::~LinuxTimerManager() {
    thread_shutdown = true;
    sem_post(&timers_fired_semaphore);
    callback_thread.join();
    sem_destroy(&timers_fired_semaphore);
}

/* This function needs to tell the LinuxTimerManager the ID of the timer
 * that fired, but it must be a bare function and can't get any additional
 * arguments (such as a LinuxTimerManager reference). The only solution I can
 * see is to use a global pointer to one instance of LinuxTimerManager. */
void timer_signal_handler(int signum, siginfo_t* info, void* ucontext) {
    timer_id_t timer_id = info->si_value.sival_int;
    tm_instance->fired_timers.enqueue(timer_id);
    sem_post(&tm_instance->timers_fired_semaphore);
}

timer_id_t LinuxTimerManager::register_timer(const int delay_ms, std::function<void(void)> callback) {
    //Create a timer that will signal with a sigev_value containing its ID
    struct sigevent timer_event = {0};
    timer_event.sigev_notify = SIGEV_SIGNAL;
    timer_event.sigev_signo = timer_signal_num;
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
    std::lock_guard<std::mutex> lock(cancelled_timers_mutex);
    if(timer_handles.find(timer_id) != timer_handles.end()) {
        cancelled_timers.emplace(timer_id);
    }
}

void LinuxTimerManager::callback_thread_main() {
    pthread_setname_np(pthread_self(), "timer_callback_thread");
    /* Block the receipt of timer signals in this thread,
     * since ConcurrentQueue is non-reentrant */
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, timer_signal_num);
    pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
    moodycamel::ConsumerToken callback_thread_token(fired_timers);
    while(!thread_shutdown) {
        sem_wait(&timers_fired_semaphore);
        //Dequeue may fail if this thread was awoken by the destructor
        timer_id_t timer_id;
        if(!fired_timers.try_dequeue(callback_thread_token, timer_id)) {
            continue;
        }
        std::unique_lock<std::mutex> cancelled_timers_lock(cancelled_timers_mutex);
        //Only fire the trigger if the timer is not in the cancelled set
        auto cancelled_location = cancelled_timers.find(timer_id);
        if(cancelled_location == cancelled_timers.end()) {
            //The callback may take a while, allow other cancellations while it is running
            cancelled_timers_lock.unlock();
            timer_callbacks.at(timer_id)();
            cancelled_timers_lock.lock();
        } else {
            cancelled_timers.erase(cancelled_location);
        }
        timer_callbacks.erase(timer_id);
        timer_delete(tm_instance->timer_handles.at(timer_id));
        timer_handles.erase(timer_id);
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

