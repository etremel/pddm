/*
 * Overlay.cpp
 *
 *  Created on: May 26, 2016
 *      Author: edward
 */

#include <map>
#include <tuple>
#include <vector>
#include <cstdint>
#include <random>
#include <stdexcept>

#include "Overlay.h"
#include "PrimeModuli.h"

namespace pddm {
namespace util {

static std::mt19937 random_engine;

/**
 * Efficient modpow implementation found on StackOverflow
 * @param num The base
 * @param pow The exponent
 * @param mod The modulus
 * @return (num ^ pow) % mod
 */
uint32_t mod_pow(uint32_t num, uint32_t pow, uint32_t mod) {
    uint64_t result = 1;
    while (pow > 0) {
        if (pow & 1)
            result = (result * num) % mod;
        num = (num * num) % mod;
        pow >>= 1;
    }
    return result;
}

int gossip_target(const int source_id, const int round, const int group_size) {
    //Memo table ordering is (N, t, i) -> i + 2^t mod N
    //This is copied from the Java version, and it may be possible to reorder the tuple
    static std::map<std::tuple<int, int, int>, int> memo_table;
    auto memo_value = memo_table.find(std::make_tuple(group_size, round, source_id));
    if(memo_value != memo_table.end()) {
        return memo_value->second;
    }
    int target = (source_id + mod_pow(2, round, group_size)) % group_size;
    memo_table.emplace(std::make_tuple(group_size, round, source_id), target);
    return target;
}

/**
 * "Safe" modular subtraction, which correctly wraps around negative values of
 * x-y (unlike the built-in % operator). Copied from StackOverflow.
 * @param x
 * @param y
 * @param m
 * @return (x - y) % m, correctly accounting for (x - y) < 0
 */
inline int mod_subtract(const int x, const int y, const int m) {
    return ((x - y) % m) + ((x >= y) ? 0 : m);
}

int gossip_predecessor(const int target_id, const int round, const int group_size) {
    //Memo table ordering is (N, t, j) -> j - 2^t mod N
    static std::map<std::tuple<int, int, int>, int> memo_table;
    auto memo_value = memo_table.find(std::make_tuple(group_size, round, target_id));
    if(memo_value != memo_table.end()) {
        return memo_value->second;
    }
    int source = mod_subtract(target_id, mod_pow(2, round, group_size), group_size);
    memo_table.emplace(std::make_tuple(group_size, round, target_id), source);
    return source;
}

inline constexpr int standard_group_size(const int num_groups, const int num_meters) {
    return num_meters / num_groups; //Divide and round down
}

inline constexpr int second_last_group_size(const int num_groups, const int num_meters) {
    int group_size = standard_group_size(num_groups, num_meters);
    int leftover_size = num_meters - (num_groups-1) * group_size;
    return (group_size + leftover_size) / 2; //rounds down, but the extra 1 will get included in the last group
}

int random_int_exclude(const int min, const int max, const int exclude) {
    int choice = std::uniform_int_distribution<>(min, max-1)(random_engine);
    return choice == exclude ? max : choice;
}

std::vector<int> pick_proxies(const int node_id, const int num_groups, const int num_meters) {
    std::vector<int> proxies(num_groups);

    int group_size = standard_group_size(num_groups, num_meters);
    int second_last_size = second_last_group_size(num_groups, num_meters);

    if(group_size < 2) {
        throw std::runtime_error("Too many groups for this system size. Each group would be size 1 or less.");
    }

    //Pick one proxy from each full-size group
    for(int group_num = 0; group_num < num_groups-2; group_num++) {
        int group_begin = group_num * group_size;
        int group_end = group_begin + (group_size - 1);
        if(group_begin <= node_id && node_id <= group_end) {
            proxies[group_num] = random_int_exclude(group_begin, group_end, node_id);
        } else {
            proxies[group_num] = std::uniform_int_distribution<>(group_begin, group_end)(random_engine);
        }
    }
    //Pick a proxy for the second-to-last group
    int group_begin = (num_groups-2) * group_size;
    int group_end = group_begin + (second_last_size - 1);
    if(group_begin <= node_id && node_id <= group_end) {
        proxies[num_groups-2] = random_int_exclude(group_begin, group_end, node_id);
    } else {
        proxies[num_groups-2] = std::uniform_int_distribution<>(group_begin, group_end)(random_engine);
    }
    //Pick a proxy for the last group
    group_begin = group_end + 1;
    group_end = num_meters-1;
    if(group_begin <= node_id && node_id <= group_end) {
        proxies[num_groups-1] = random_int_exclude(group_begin, group_end, node_id);
    } else {
        proxies[num_groups-1] = std::uniform_int_distribution<>(group_begin, group_end)(random_engine);
    }

    return proxies;
}

int aggregation_group_for(const int node_id, const int num_groups, const int num_meters) {
    int group_size = standard_group_size(num_groups, num_meters);
    int secondLastGroupSize = second_last_group_size(num_groups, num_meters);
    //Divide the ID by the group size and round down, because groups start at 0 (so IDs between 0 and group_size are in group 0)
    int group_num = node_id / group_size;
    //The last 2 groups might not be the right size, so explicitly check if it's in the second-to-last or last group
    if(group_num >= num_groups-2) {
        if(node_id >= group_size * (num_groups-2)
                && node_id < group_size * (num_groups-2) + secondLastGroupSize) {
            group_num = num_groups-2;
        } else {
            group_num = num_groups-1;
        }
    }
    return group_num;
}

int aggregation_tree_parent(const int node_id, const int num_groups, const int num_meters) {
    int group_size = standard_group_size(num_groups, num_meters);
    int firstID;
    if(aggregation_group_for(node_id, num_groups, num_meters) == num_groups-1) {
        //The last group has a different first member due to the second-to-last group's uneven size
        int secondLastGroupSize = second_last_group_size(num_groups, num_meters);
        firstID = (num_groups-2) * group_size + secondLastGroupSize;
    } else {
        firstID = aggregation_group_for(node_id, num_groups, num_meters) * group_size;
    }
    if(node_id == firstID) {
        return -1;
    }
    int parentRelative = (node_id - firstID - 1) / 2;
    return parentRelative + firstID;
}

std::pair<int, int> aggregation_tree_children(const int node_id, const int num_groups, const int num_meters) {
    int group_size = standard_group_size(num_groups, num_meters);
    int secondLastGroupSize = second_last_group_size(num_groups, num_meters);
    int aggregation_group = aggregation_group_for(node_id, num_groups, num_meters);
    int firstID;
    if(aggregation_group == num_groups-1) {
        //The last group has a different first member due to the second-to-last group's uneven size
        firstID = (num_groups-2) * group_size + secondLastGroupSize;
        //Update group_size
        group_size = num_meters - firstID;
    } else if(aggregation_group == num_groups-2){
        //If it's the second-to-last group, update group_size, but the first member is the same
        firstID = aggregation_group * group_size;
        group_size = secondLastGroupSize;
    } else {
        firstID = aggregation_group * group_size;
    }

    int rightChildRelative = (node_id - firstID + 1) * 2;
    int leftChildRelative = rightChildRelative - 1;
    if(leftChildRelative >= group_size) {
        return std::make_pair(-1, -1);
    }
    if(rightChildRelative >= group_size) {
        return std::make_pair(leftChildRelative + firstID, -1);
    }

    return std::make_pair(leftChildRelative + firstID, rightChildRelative + firstID);

}

int get_valid_prime_modulus(const int lower_bound) {
    auto result = std::lower_bound(valid_prime_moduli.begin(), valid_prime_moduli.end(), lower_bound);
    return *result;
}

}

}
