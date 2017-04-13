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

#include "TimerManager.h"

namespace pddm {
class UtilityClient;
class MeterClient;
}

namespace pddm {
namespace util {

class LinuxTimerManager: public TimerManager {
    private:
         timer_id_t next_id;
         std::map<timer_id_t, timer_t> timer_handles;
         std::map<timer_id_t, std::function<void(void)>> timer_callbacks;
         friend void timer_signal_handler(int signum, siginfo_t* info, void* ucontext);
    public:
        LinuxTimerManager();
        virtual ~LinuxTimerManager() = default;
        timer_id_t register_timer(const int delay_ms, std::function<void(void)> callback) override;
        void cancel_timer(const timer_id_t timer_id) override;
};

void timer_signal_handler(int signum, siginfo_t* info, void* ucontext);

std::function<LinuxTimerManager (MeterClient&)> timer_manager_builder();
std::function<LinuxTimerManager (UtilityClient&)> timer_manager_builder_utility();


} /* namespace util */
} /* namespace pddm */

