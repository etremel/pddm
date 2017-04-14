/*
 * Meter.cpp
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#include "Meter.h"

#include <algorithm>
#include <list>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include "../FixedPoint_t.h"
#include "SimParameters.h"
#include "Timesteps.h"

namespace pddm {
namespace simulation {

using std::string;
using std::vector;
using std::make_pair;
using namespace timesteps;

inline bool device_already_picked(const std::list<Device>& existing_devices, const std::regex& name_pattern) {
    for(const auto& existing_device : existing_devices) {
        if(std::regex_search(existing_device.name, name_pattern))
            return true;
    }
    return false;
}

std::unique_ptr<Meter> generate_meter(std::discrete_distribution<>& income_distribution,
        const std::map<std::string, Device>& possible_devices,
        const std::map<std::string, double>& devices_saturation,
        const PriceFunction& energy_price_function,
        std::mt19937& random_engine) {
    int income_choice = income_distribution(random_engine);
    IncomeLevel income_level = income_choice == 0 ? IncomeLevel::POOR :
            (income_choice == 1 ? IncomeLevel::AVERAGE : IncomeLevel::RICH);
    //Pick what devices this home owns based on their saturation percentages
    std::list<Device> home_devices;
    for(const auto& device_saturation : devices_saturation) {
        //Devices that end in digits have multiple "versions," and only one of them should be in home_devices
        std::regex ends_in_digits(".*[0-9]$", std::regex::extended);
        if(std::regex_match(device_saturation.first, ends_in_digits)) {
            std::regex name_prefix(device_saturation.first.substr(0, device_saturation.first.length()-2) + string(".*"), std::regex::extended);
            if(device_already_picked(home_devices, name_prefix)) {
                continue;
            }

        }
        //Homes have either a window or central AC but not both
        std::regex conditioner("conditioner");
        if(std::regex_search(device_saturation.first, conditioner) && device_already_picked(home_devices, conditioner)) {
            continue;
        }
        //Otherwise, randomly decide whether to include this device, based on its saturation
        double saturation_as_fraction = device_saturation.second / 100.0;
        if(std::bernoulli_distribution(saturation_as_fraction)(random_engine)) {
            home_devices.emplace_back(possible_devices.at(device_saturation.first));
        }
    }
    return std::make_unique<Meter>(income_level, home_devices, energy_price_function);
}

Meter::Meter(const IncomeLevel& income_level, std::list<Device>& owned_devices, const PriceFunction& energy_price_function) :
        energy_price_function(energy_price_function), income_level(income_level), current_timestep(-1),
        consumption(TOTAL_TIMESTEPS), shiftable_consumption(TOTAL_TIMESTEPS), cost(TOTAL_TIMESTEPS) {
    for(auto& device : owned_devices) {
        //Currently, only air conditioners are shiftable (smart thermostats)
        if(device.name.find("conditioner") != string::npos) {
            shiftable_devices.emplace_back(std::move(device), DeviceState{});
        } else {
            nonshiftable_devices.emplace_back(std::move(device), DeviceState{});
        }
    }

}

/**
 * Simulates one timestep of energy usage and updates the Meter's variables
 * with the results.
 */
void Meter::simulate_usage_timestep() {
    current_timestep++;

    consumption[current_timestep] = simulate_nonshiftables(current_timestep, energy_price_function);
    shiftable_consumption[current_timestep] = simulate_shiftables(current_timestep, energy_price_function);
    consumption[current_timestep] += shiftable_consumption[current_timestep];

    //If the next timestep will be after midnight of a new day,
    //reset actual start time to "not yet run" for devices that have finished running today
    if(hour(current_timestep+1) % 24 == 0) {
        for(auto& device_pair : shiftable_devices) {
            DeviceState& deviceState = device_pair.second;
            if(deviceState.is_on == false && deviceState.start_time != -1) {
                deviceState.start_time = -1;
                deviceState.current_cycle_num = 0;
            }
        }
        for(auto& device_pair : nonshiftable_devices) {
            DeviceState& deviceState = device_pair.second;
            if(deviceState.is_on == false && deviceState.start_time != -1) {
                deviceState.start_time = -1;
                deviceState.current_cycle_num = 0;
            }
        }
    }
}


FixedPoint_t Meter::simulate_nonshiftables(int time, const PriceFunction& energy_price) {
    FixedPoint_t total_consumption;
    for(auto& device_pair : nonshiftable_devices) {
        double step_factor = USAGE_TIMESTEP_MIN / 60.0;
        double hourly_factor;
        double frequency_factor;
        if(day(time) % 7 == 5 || day(time % 7 == 6)) {
            hourly_factor = device_pair.first.weekend_hourly_probability[hour(time) % 24];
            frequency_factor = device_pair.first.weekend_frequency;
        } else {
            hourly_factor = device_pair.first.weekday_hourly_probability[hour(time) % 24];
            frequency_factor = device_pair.first.weekday_frequency;
        }
        //Probability of starting = step_factor * hourly_factor * frequency_factor
        if(std::bernoulli_distribution{step_factor * hourly_factor * frequency_factor}(random_engine)) {
            device_pair.second.is_on = true;
        }
        if(device_pair.second.is_on) {
            device_pair.second.start_time = time;
            total_consumption += run_device(device_pair.first, device_pair.second);
        }
        //Regardless of whether device turned on, add its standby usage
        total_consumption += device_pair.first.standby_load * FixedPoint_t(step_factor);
    }
    return total_consumption;
}

FixedPoint_t Meter::simulate_shiftables(int time, const PriceFunction& energy_price) {
    FixedPoint_t total_consumption;
    for(auto& device_pair : shiftable_devices) {
        if(device_pair.second.scheduled_start_time > -1) {
            if(!device_pair.second.is_on && time >= device_pair.second.scheduled_start_time) {
                device_pair.second.is_on = true;
                device_pair.second.start_time = time;
            }
        } else {
            double step_factor = USAGE_TIMESTEP_MIN / 60.0;
             double hourly_factor;
             double frequency_factor;
             if(day(time) % 7 == 5 || day(time % 7 == 6)) {
                 hourly_factor = device_pair.first.weekend_hourly_probability[hour(time) % 24];
                 frequency_factor = device_pair.first.weekend_frequency;
             } else {
                 hourly_factor = device_pair.first.weekday_hourly_probability[hour(time) % 24];
                 frequency_factor = device_pair.first.weekday_frequency;
             }
             //Probability of starting = step_factor * hourly_factor * frequency_factor
             if(std::bernoulli_distribution{step_factor * hourly_factor * frequency_factor}(random_engine)) {
                 device_pair.second.is_on = true;
             }
        }
        if(device_pair.second.is_on) {
            total_consumption += run_device(device_pair.first, device_pair.second);
        }
        //If the device was scheduled and has just completed its run, reset it to non-scheduled
        //Note that run_device sets is_on back to false if the device finished running during this timestep
        if(time >= device_pair.second.scheduled_start_time && !device_pair.second.is_on) {
            device_pair.second.scheduled_start_time = -1;
        }
        //Regardless of whether device turned on, add its standby usage
        total_consumption += device_pair.first.standby_load * FixedPoint_t(USAGE_TIMESTEP_MIN / 60.0);
    }
    return total_consumption;
}

/**
 * Simulates a single device for a single timestep of time, and returns the
 * amount of power in watt-hours that device used during the timestep. The
 * device's state will be updated to reflect the cycle it's in at the end
 * of the timestep, if it's a device with multiple cycles. The device is
 * assumed to be on when this method is called ({@code device.isOn == true}),
 * since it doesn't make sense to call this method on a device that is off.
 *
 * @param device A device object
 * @param device_state The device's associated DeviceState object
 * @return The number of watt-hours of power consumed by this device in
 * the simulated timestep
 */
FixedPoint_t Meter::run_device(Device& device, DeviceState& device_state) {
    FixedPoint_t power_consumed; //in watt-hours
    int time_remaining_in_timestep = USAGE_TIMESTEP_MIN;
    //Simulate as many device cycles as will fit in one timestep
    while(time_remaining_in_timestep > 0
            && device_state.current_cycle_num < (int) device.load_per_cycle.size()) {
        //The device may already be partway through the current cycle
        int current_cycle_time = device.time_per_cycle[device_state.current_cycle_num] - device_state.time_in_current_cycle;
        //Simulate a partial cycle if the current cycle has more time remaining than the timestep
        int minutes_simulated = std::min(current_cycle_time, time_remaining_in_timestep);
        power_consumed += device.load_per_cycle[device_state.current_cycle_num] * FixedPoint_t(minutes_simulated / 60.0);
        device_state.time_in_current_cycle += minutes_simulated;
        time_remaining_in_timestep -= minutes_simulated;
        //If we completed simulating a cycle, advance to the next one and check if there's time remaining in the timestep
        if(device_state.time_in_current_cycle == device.time_per_cycle[device_state.current_cycle_num]) {
            device_state.current_cycle_num++;
            device_state.time_in_current_cycle = 0;
        }
    }
    //If the loop stopped because the device finished its last cycle, turn it off
    if(device_state.current_cycle_num == (int) device.load_per_cycle.size()) {
        device_state.is_on = false;
        device_state.current_cycle_num= 0;
    }
    return power_consumed;
}

std::vector<FixedPoint_t> Meter::simulate_projected_usage(const PriceFunction& projected_price, const int time_window) {
    int window_whole_timesteps = time_window / USAGE_TIMESTEP_MIN;
    FixedPoint_t window_last_fraction_timestep(time_window / (double) USAGE_TIMESTEP_MIN - window_whole_timesteps);
    std::vector<FixedPoint_t> projected_usage(window_whole_timesteps + 1);
    //save states of devices, which will be modified by the "fake" simulation
    std::vector<std::pair<Device, DeviceState>> shiftable_backup(shiftable_devices);
    std::vector<std::pair<Device, DeviceState>> nonshiftable_backup(nonshiftable_devices);
    //simulate the next time_window minutes under the proposed function
    for(int sim_ts = current_timestep; sim_ts < current_timestep + window_whole_timesteps + 1; ++sim_ts) {
        projected_usage[sim_ts-current_timestep] = simulate_nonshiftables(sim_ts, projected_price)
                + simulate_shiftables(sim_ts, projected_price);
    }
    projected_usage[window_whole_timesteps] *= window_last_fraction_timestep;
    //restore saved states
    std::swap(shiftable_devices, shiftable_backup);
    std::swap(nonshiftable_devices, nonshiftable_backup);
    return projected_usage;
}

FixedPoint_t Meter::measure(const std::vector<FixedPoint_t>& data, const int window_minutes) const {
    FixedPoint_t window_consumption;
    int windowWholeTimesteps = window_minutes / USAGE_TIMESTEP_MIN;
    FixedPoint_t windowLastFractionTimestep(window_minutes / (double) USAGE_TIMESTEP_MIN - windowWholeTimesteps);
    for(int offset = 0; offset < windowWholeTimesteps; offset++) {
        window_consumption += data[current_timestep-offset];
    }
    window_consumption += (data[current_timestep-windowWholeTimesteps] * windowLastFractionTimestep);
    return window_consumption;
}

FixedPoint_t Meter::measure_consumption(const int window_minutes) const {
    return measure(consumption, window_minutes);
}

FixedPoint_t Meter::measure_shiftable_consumption(const int window_minutes) const {
    return measure(shiftable_consumption, window_minutes);
}

FixedPoint_t Meter::measure_daily_consumption() const {
    int day_start = timesteps::day(current_timestep) * (1440 / USAGE_TIMESTEP_MIN);
    FixedPoint_t daily_consumption;
    for(int time = day_start; time < current_timestep; ++time) {
        daily_consumption += consumption[time];
    }
    return daily_consumption;
}

} /* namespace simulation */
} /* namespace psm */

