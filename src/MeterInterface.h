/*
 * MeterInterface.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include <vector>

#include "FixedPoint_t.h"
#include "util/Money.h"

namespace pddm {

using PriceFunction = std::function<Money (const int)>;

/**
 * Defines the interface for a meter device (the hardware itself), used by the
 * MeterClient to get measurements.
 */
class MeterInterface {
    public:
        virtual ~MeterInterface() = 0;
        virtual std::vector<FixedPoint_t> simulate_projected_usage(const PriceFunction& projected_price, const int time_window) = 0;
        virtual FixedPoint_t measure_consumption(const int window_minutes) const = 0;
        virtual FixedPoint_t measure_shiftable_consumption(const int window_minutes) const = 0;
        virtual FixedPoint_t measure_daily_consumption() const = 0;

};

inline MeterInterface::~MeterInterface() { }

}


