/*
 * Meter.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <vector>

#include "../Configuration.h"
#include "SimParameters.h"
#include "Device.h"
#include "DeviceState.h"
#include "../util/Money.h"

namespace pddm {
namespace simulation {

class Meter : public MeterInterface {
    private:
        std::vector<std::pair<Device, DeviceState>> shiftable_devices;
        std::vector<std::pair<Device, DeviceState>> nonshiftable_devices;
        PriceFunction energy_price_function;
        int current_timestep;
        std::vector<FixedPoint_t> consumption;
        std::vector<FixedPoint_t> shiftable_consumption;
        std::vector<Money> cost;

        FixedPoint_t simulate_nonshiftables(int time, const PriceFunction& energy_price);
        FixedPoint_t simulate_shiftables(int time, const PriceFunction& energy_price);
        FixedPoint_t run_device(Device& device, DeviceState& device_state);
        FixedPoint_t measure(const std::vector<FixedPoint_t>& data, const int window_minutes) const;

    public:
        Meter(std::vector<Device>& owned_devices, const PriceFunction& energy_price_function);
        std::vector<FixedPoint_t> simulate_projected_usage(const PriceFunction& projected_price, int time_window) override;
        FixedPoint_t measure_consumption(const int window_minutes) const override;
        FixedPoint_t measure_shiftable_consumption(const int window_minutes) const override;
        FixedPoint_t measure_daily_consumption() const override;
        /** Simulates one timestep of energy usage and updates the internal vectors. */
        void simulate_usage_timestep();
};


MeterBuilderFunc meter_builder(std::vector<Device>& owned_devices, const Meter::PriceFunction& energy_price_function);

} /* namespace simulation */
} /* namespace psm */

