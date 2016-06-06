/*
 * Meter.cpp
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#include <vector>
#include <string>

#include "Meter.h"
#include "Device.h"
#include "Timesteps.h"

namespace pddm {
namespace simulation {

using std::string;
using std::vector;
using std::make_pair;
using namespace timesteps;

Meter::Meter(std::list<Device>& owned_devices, const PriceFunction& energy_price_function, const IncomeLevel& income_level) :
        energy_price_function(energy_price_function), income_level(income_level), current_timestep(-1),
        consumption(TOTAL_TIMESTEPS), shiftable_consumption(TOTAL_TIMESTEPS), cost(TOTAL_TIMESTEPS) {
    for(auto& device : owned_devices) {
        //Currently, only air conditioners are shiftable (smart thermostats)
        if(device.name.find("conditioner") != string::npos) {
            shiftable_devices.push_back(make_pair(std::move(device), DeviceState{}));
        } else {
            nonshiftable_devices.push_back(make_pair(std::move(device), DeviceState{}));
        }
    }

}


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

FixedPoint_t Meter::measure(const std::vector<FixedPoint_t>& data, const int window_minutes) const {
    FixedPoint_t windowConsumption;
    int windowWholeTimesteps = window_minutes / USAGE_TIMESTEP_MIN;
    FixedPoint_t windowLastFractionTimestep(window_minutes / (double) USAGE_TIMESTEP_MIN - windowWholeTimesteps);
    for(int offset = 0; offset < windowWholeTimesteps; offset++) {
        windowConsumption += data[current_timestep-offset];
    }
    windowConsumption += (data[current_timestep-windowWholeTimesteps] * windowLastFractionTimestep);
    return windowConsumption;
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


MeterBuilderFunc meter_builder(const IncomeLevel& income_level, std::list<Device>& owned_devices,
        const Meter::PriceFunction& energy_price_function, std::vector<std::reference_wrapper<Meter>>& meter_references) {
    return [&income_level, &owned_devices, &energy_price_function, &meter_references](MeterClient& client) {
        Meter new_meter(income_level, owned_devices, energy_price_function);
        //Assuming MeterClients are created in strict ID order, this will put a reference to the Meter
        //at the same index as the ID of the new MeterClient that owns it
        meter_references.push_back(std::ref(new_meter));
        return new_meter;
    };
}

} /* namespace simulation */
} /* namespace psm */

