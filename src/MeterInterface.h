/*
 * MeterInterface.h
 *
 *  Created on: May 17, 2016
 *      Author: edward
 */

#pragma once

#include <vector>

#include "Configuration.h"

namespace pddm {

/**
 * Defines the interface for a meter device (the hardware itself), used by the
 * MeterClient to get measurements.
 */
class MeterInterface {
        using PriceFunction = std::function<Money (int)>;
    public:
        virtual ~MeterInterface() = 0;
        virtual std::vector<FixedPoint_t> simulate_projected_usage(const PriceFunction& projected_price, int time_window);
        virtual FixedPoint_t measure_consumption(const int window_minutes);
        virtual FixedPoint_t measure_shiftable_consumption(const int window_minutes);
        virtual FixedPoint_t measure_daily_consumption();

};

}


