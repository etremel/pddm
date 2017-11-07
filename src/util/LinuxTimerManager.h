/**
 * @file LinuxTimerManager.h
 *
 * @date Oct 7, 2016
 * @author edward
 */

#pragma once

#include <ctime>
#include <csignal>
#include <functional>
#include <map>
#include <set>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <moodycamel/concurrentqueue.h>

#include "TimerManager.h"

namespace pddm {
class UtilityClient;
class MeterClient;
}

namespace pddm {
namespace util {

class LinuxTimerManager: public TimerManager {
    private:
        const int timer_signal_num;
        /** The next ID that will be returned on a call to register_timer */
        timer_id_t next_id;
        /** Maps each timer ID to its Linux-generated "handle" struct. */
        std::map<timer_id_t, timer_t> timer_handles;
        /** Maps each timer ID to the user-provided callback function it should execute. */
        std::map<timer_id_t, std::function<void(void)>> timer_callbacks;
        /** A set of timer IDs that are cancelled. */
        std::set<timer_id_t> cancelled_timers;
        /** A list of timer IDs that have fired and need to have their callbacks
         * executed, in order of when they fired. Written by the timer signal
         * handler, read by the callback-executing thread. */
        moodycamel::ConcurrentQueue<timer_id_t> fired_timers;
        /** Semaphore used by the timer signal handler to safely tell the
         * callback-executing thread that a timer has fired. */
        sem_t timers_fired_semaphore;
        /** Synchronizes access to cancelled_timers between the callback-executing
         * thread and cancel_timer(). */
        std::mutex cancelled_timers_mutex;
        /** Shuts down the callback-executing thread when true. */
        std::atomic<bool> thread_shutdown;
        /** The thread that actually runs timer callbacks once the timers fire.
         * This must be a separate thread because Linux timer events are delivered
         * to an interrupt handler, which must be nonblocking. */
        std::thread callback_thread;
        void callback_thread_main();
        friend void timer_signal_handler(int signum, siginfo_t* info, void* ucontext);
    public:
        LinuxTimerManager();
        virtual ~LinuxTimerManager();
        timer_id_t register_timer(const int delay_ms, std::function<void(void)> callback) override;
        void cancel_timer(const timer_id_t timer_id) override;
};

void timer_signal_handler(int signum, siginfo_t* info, void* ucontext);

std::function<LinuxTimerManager (MeterClient&)> timer_manager_builder();
std::function<LinuxTimerManager (UtilityClient&)> timer_manager_builder_utility();


} /* namespace util */
} /* namespace pddm */

