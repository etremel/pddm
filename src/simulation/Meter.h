/*
 * Meter.h
 *
 *  Created on: May 16, 2016
 *      Author: edward
 */

#pragma once

#include <functional>
#include <list>
#include <map>
#include <memory>
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

/**
 * Factory that creates a new simulated meter by picking a set of devices at
 * random, based on their saturation percentages and an income distribution.
 * @param income_distribution The percentages of low, middle, and high-income
 * homes in the region being simulated
 * @param possible_devices The set of possible devices, indexed by name
 * @param devices_saturation Maps a device name to the saturation of that device,
 * as a percentage.
 * @param energy_price_function The price function to store in the created meter.
 * @param random_engine A source of randomness. Specified to be std::mt19937
 * because (bizarrely) there's no common supertype for randomness engines, and
 * this is the one I happen to use in Simulator.
 * @return A new Meter with a random set of devices
 */
std::unique_ptr<Meter> generate_meter(std::discrete_distribution<>& income_distribution,
        const std::map<std::string, Device>& possible_devices,
        const std::map<std::string, double>& devices_saturation,
        const PriceFunction& energy_price_function,
        std::mt19937& random_engine);

} /* namespace simulation */
} /* namespace psm */

