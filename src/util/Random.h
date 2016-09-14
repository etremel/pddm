/**
 * @file Random.h
 * Utilities for generating random numbers in formats other than the ones
 * supported by the standard library.
 * @date Sep 12, 2016
 * @author edward
 */

#pragma once

#include <vector>
#include <algorithm>
#include <type_traits>
#include <random>

namespace pddm {
namespace util {

/**
 * Samples integers from a range of values, picking without replacement, while
 * excluding certain values within that range.
 * @param range_begin The beginning of the range to sample (inclusive).
 * @param range_end The end of the range to sample, exclusive.
 * @param exclude_values A sorted list of integers between range_begin and
 * range_end to be excluded from the sample. This must be sorted in order for
 * the algorithm to behave correctly.
 * @param number_to_sample The number of integers to pick.
 * @param random_engine The source of randomness to use.
 * @return number_to_sample distinct integers from the range [range_begin, range_end),
 * excluding all values in exclude_values.
 */
template<typename IntT, typename Generator,
typename = std::enable_if_t<std::is_integral<IntT>::value>>
std::vector<IntT> sample_range_excluding(const IntT range_begin, const IntT range_end,
        const std::vector<IntT>& exclude_values, const typename std::vector<IntT>::size_type number_to_sample,
        Generator& random_engine) {
    std::vector<IntT> range(range_end-range_begin);
    std::iota(range.begin(), range.end(), range_begin);
    std::vector<IntT> filtered_range;
    std::set_intersection(range.begin(), range.end(),
            exclude_values.begin(), exclude_values.end(),
            std::back_inserter(filtered_range));
    std::shuffle(filtered_range.begin(), filtered_range.end(), random_engine);
    filtered_range.resize(number_to_sample);
    return filtered_range;
}

/**
 * Picks {@code num_values} integers in the range [0, Max) without replacement.
 * Uses a uniform int distribution sampled using the provided source of
 * randomness.
 * @param max The upper bound on values to pick, exclusive.
 * @param num_values The number of distinct integers to sample.
 * @param random_engine The source of randomness to use.
 * @return num_values distinct random integers less than max.
 */
template<typename IntT, typename Generator,
typename = std::enable_if_t<std::is_integral<IntT>::value>>
std::vector<IntT> pick_without_replacement(const IntT max, const IntT num_values, Generator& random_engine) {
    std::vector<bool> picked(max);
    std::uniform_int_distribution<IntT> random(0, max);
    int num_picked = 0;
    while(num_picked < num_values) {
        IntT value = random(random_engine);
        if(!picked[value]) {
            picked[value] = true;
            num_picked++;
        }
    }
    std::vector<IntT> chosen_values;
    for(IntT val = 0; val < max; ++val) {
        if(picked[val]) {
            chosen_values.emplace_back(val);
        }
    }
    return chosen_values;
}

}
}


