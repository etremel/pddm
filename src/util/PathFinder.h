/*
 * PathFinder.h
 *
 *  Created on: May 26, 2016
 *      Author: edward
 */

#pragma once

#include <vector>
#include <list>

namespace pddm {
namespace util {

/**
 * Finds node-disjoint paths from the source node to the target nodes in
 * an instance of Bobby's gossip graph, using a breadth-first search,
 * where the source starts sending at the given round. Paths are returned as a
 * sequence of node IDs, assuming that nodes can figure out how many rounds
 * to wait before forwarding the message to the next ID in the sequence.
 *
 * @param source_id The ID of the source node
 * @param target_ids The IDs of the target nodes
 * @param num_nodes The number of nodes in the graph (i.e. the modulus size)
 * @param start_round The round number on which the source node wants to start
 *        sending messages
 * @return A vector of paths, in the same order as the list of target IDs,
 *         where each path is a list of node IDs in time order.
 *         This list does not include the source, but does include the target.
 */
std::vector<std::list<int>> find_paths(const int source_id, const std::vector<int>& target_ids,
        const int num_nodes, const int start_round);

}
}
