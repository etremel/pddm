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

/**
 * Calculates the gossip target for a node in the given round by evaluating
 * g(i,t) = i + 2^t mod N, where N is the number of nodes in the system.
 *
 * @param source_id The ID of the source node.
 * @param round The current round number (time)
 * @param group_size The total number of nodes in the system.
 * @return The ID of the node's gossip target in the current round.
 */
int gossip_target(const int source_id, const int round, const int group_size);

/**
 * Calculates the predecessor of a node in the overlay graph by evaluating
 * g^-1(j,t) = j - 2^t mod N, where N is the number of nodes in the system.
 *
 * @param target_id The ID of the target node
 * @param round The current round number (time)
 * @param group_size The total number of nodes in the system
 * @return The ID of the node's gossip predecessor in the current round
 */
int gossip_predecessor(const int target_id, const int round, const int group_size);

/**
 * Picks a set of proxies for a node with the given ID by randomly picking
 * one ID from each aggregation group, where aggregation groups are sequences
 * of {@code numMeters} / {@code numGroups} consecutive IDs.
 *
 * @param node_id The ID of the node for which proxies should be picked
 * @param num_groups The number of aggregation groups
 * @param num_meters The total number of meters in the system
 * @return A randomly chosen vector of {@code numGroups} proxy IDs
 */
std::vector<int> pick_proxies(const int node_id, const int num_groups, const int num_meters);

/**
 * Computes the aggregation group number (zero-indexed) for a given node ID
 * @param node_id
 * @param num_groups The number of aggregation groups
 * @param num_meters The total number of nodes in the system
 * @return
 */
int aggregation_group_for(const int node_id, const int num_groups, const int num_meters);

/**
 * Computes the ID of the given node's parent in a binary tree within its
 * aggregation group. The lowest-numbered ID in a group is the root of the
 * tree, the next two IDs are the first level, the next four IDs are the
 * second level, etc.
 * @param node_id The ID of the node for which to compute the parent.
 * @param num_groups The number of aggregation groups
 * @param num_meters The total number of nodes in the system
 * @return The ID of {@code nodeID}'s parent, or -1 if {@code nodeID} is
 *         the root of the tree.
 */
int aggregation_tree_parent(const int node_id, const int num_groups, const int num_meters);

/**
 * Computes the IDs of the given node's children in a binary tree within its
 * aggregation group. The left child is the first element of the returned
 * pair, and the right child is the second element. If either child does
 * not exist (because it is outside the range of the aggregation group),
 * that element will be -1.
 * @param node_id The ID of the node for which to compute the children.
 * @param num_groups The number of aggregation groups
 * @param num_meters The total number of nodes in the system
 * @return A std::pair containing the children's node IDs
 */
std::pair<int, int> aggregation_tree_children(const int node_id, const int num_groups, const int num_meters);

/**
 * Finds the smallest prime modulus larger than <code>lowerBound</code> that will
 * create a field over the integers with primitive root 2. Uses a pre-computed list
 * of valid prime moduli, from http://oeis.org/A001122.
 * @param lower_bound The value to find a prime modulus near
 * @return The smallest valid prime modulus that is not less than {@code lower_bound}
 */
int get_valid_prime_modulus(const int lower_bound);

}
}


