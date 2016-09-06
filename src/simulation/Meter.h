/*
 * Meter.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <functional>
#include <list>
#include <random>
#include <utility>
#include <vector>

#include "../MeterInterface.h"
#include "../FixedPoint_t.h"
#include "../util/Money.h"
#include "Device.h"
#include "DeviceState.h"
#include "IncomeLevel.h"

namespace pddm {

class MeterClient;

namespace simulation {

/**
 * Represents a simulated meter, which generates simulated electrical consumption
 * measurements based on a list of devices in the "home" it is attached to.
 */
class Meter : public MeterInterface {
    private:
        std::vector<std::pair<Device, DeviceState>> shiftable_devices;
        std::vector<std::pair<Device, DeviceState>> nonshiftable_devices;
        PriceFunction energy_price_function;
        IncomeLevel income_level;
        int current_timestep;
        std::vector<FixedPoint_t> consumption;
        std::vector<FixedPoint_t> shiftable_consumption;
        std::vector<Money> cost;
        std::mt19937 random_engine;

        FixedPoint_t simulate_nonshiftables(int time, const PriceFunction& energy_price);
        FixedPoint_t simulate_shiftables(int time, const PriceFunction& energy_price);
        FixedPoint_t run_device(Device& device, DeviceState& device_state);
        FixedPoint_t measure(const std::vector<FixedPoint_t>& data, const int window_minutes) const;

    public:
        Meter(const IncomeLevel& income_level, std::list<Device>& owned_devices, const PriceFunction& energy_price_function);
        virtual ~Meter() = default;
        std::vector<FixedPoint_t> simulate_projected_usage(const PriceFunction& projected_price, const int time_window) override;
        FixedPoint_t measure_consumption(const int window_minutes) const override;
        FixedPoint_t measure_shiftable_consumption(const int window_minutes) const override;
        FixedPoint_t measure_daily_consumption() const override;
        /** Simulates one timestep of energy usage and updates the internal vectors. */
        void simulate_usage_timestep();
};


std::function<Meter (MeterClient&)> meter_builder(const IncomeLevel& income_level, std::list<Device>& owned_devices,
        const PriceFunction& energy_price_function, std::vector<std::reference_wrapper<Meter>>& meter_references);

} /* namespace simulation */
} /* namespace psm */

