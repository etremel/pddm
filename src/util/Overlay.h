/*
 * Overlay.h
 *
 *  Created on: May 20, 2016
 *      Author: edward
 */

#pragma once

#include <vector>
#include <utility>

namespace pddm {
namespace util {

int gossip_target(const int source_id, const int round, const int group_size);

int gossip_predecessor(const int target_id, const int round, const int group_size);

std::vector<int> pick_proxies(const int node_id, const int num_groups, const int num_meters);

int aggregation_group_for(const int node_id, const int num_group, const int num_meters);

int aggregation_tree_parent(const int node_id, const int num_groups, const int num_meters);

std::pair<int, int> aggregation_tree_children(const int node_id, const int num_groups, const int num_meters);

int get_valid_prime_modulus(const int lower_bound);

}
}


