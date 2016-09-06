/*
 * PathFinder.cpp
 *
 *  Created on: May 26, 2016
 *      Author: edward
 */

#include "PathFinder.h"
#include "Overlay.h"

#include <algorithm>
#include <functional>
#include <cmath>
#include <cstddef>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using std::list;
using std::set;

namespace pddm {
namespace util {

const int MIN_PATH_LENGTH = 3;

list<int> find_another(const int source, const int target, const int n, const int starting_round, const int max_round, set<int>& used_nodes);
list<int> find_path(const int source, const int target, const int n, const int starting_round, const int max_round, const set<int>& exclude_nodes);

//Trivial helper struct for the find_path function. Note that its hash code is defined down at the bottom.
struct InfectedNode {
        int id;
        int infected_on_round;
        InfectedNode* parent;
};

inline bool operator==(const InfectedNode& lhs, const InfectedNode& rhs) {
    return lhs.id == rhs.id;
}

inline bool operator!=(const InfectedNode& lhs, const InfectedNode& rhs) {
    return !(lhs == rhs);
}

} /* namespace util */
} /* namespace pddm */

//Definition of the hash code for InfectedNode
namespace std {
template<>
struct hash<pddm::util::InfectedNode> {
        size_t operator()(const pddm::util::InfectedNode& node) const {
            return node.id;
        }
};

} /* namespace std */

namespace pddm {
namespace util {

std::vector<std::list<int>> find_paths(const int source_id, const std::vector<int>& target_ids, const int num_nodes, const int start_round) {
    set<int> used_nodes(target_ids.begin(), target_ids.end());
    std::vector<list<int>> paths(target_ids.size());
    int rounds_limit = static_cast<int>(ceil(log2(num_nodes))) * target_ids.size() + MIN_PATH_LENGTH;
    for(size_t i = 0; i < target_ids.size(); ++i) {
        paths[i] = find_another(source_id, target_ids[i], num_nodes, start_round, start_round + rounds_limit, used_nodes);
        paths[i].pop_front();
    }
    return paths;
}

/**
 * Finds and returns another path from {@code source} to {@code target}
 * through the overlay graph, that does not contain any nodes used on a
 * previously found path in the same call to {{@link #findPaths(int, List, int, int)}}.
 * A path lists both the node ID and the round number on which it arrives
 * there.
 * @param source The ID of the source node
 * @param target The ID of the target node
 * @param n The total number of nodes in the graph
 * @param starting_round The round number on which the path starts
 * @param max_round The maximum round number the path can extend to
 * @param used_nodes A reference to the set of nodes that have already been
 *        used in previously found paths.
 * @return A path from {@code source} to {@code target}, including {@code source}
 *         but not {@code target}
 */
list<int> find_another(const int source, const int target, const int n, const int starting_round, const int max_round, set<int>& used_nodes) {
    if(target >= n) {
        throw std::runtime_error(std::string("Invalid node number supplied to find_path: ") + std::to_string(target));
    }
    list<int> path = find_path(source, target, n, starting_round, max_round, used_nodes);
    for(const auto& hop : path) {
        if(hop != source && hop != target) {
            used_nodes.insert(hop);
        }
    }
    return path;
}

/**
 * Finds a path from {@code source} to {@code target} by propagating an
 * infection from {@code source} in the gossip graph, keeping track of the
 * "parent" of each node as the node that first infected it.
 * @param source
 * @param target
 * @param n The total number of nodes in the graph
 * @param starting_round The round number on which the path starts
 * @param max_round The maximum round number the path can be extended to before
 *        giving up on finding the target
 * @param exclude_nodes The set of nodes that have already been used in previous
 *        paths (note: this should be a reference to an object that was
 *        created in the initial findPaths call, so all invocations of this
 *        method in the same findPaths call are referring to the same object)
 * @return The entire path
 */
list<int> find_path(const int source, const int target, const int n, const int starting_round, const int max_round, const set<int>& exclude_nodes) {
    std::unordered_set<InfectedNode> infected;
    infected.insert(InfectedNode{source, starting_round, nullptr});
    //Propagate the infection one round at a time; this loop should not finish if the target can be reached
    for (int time = starting_round; time < max_round; time++) {
        std::unordered_set<InfectedNode> newInfectedNodes;
        for(const auto& infectedNode : infected) {
            InfectedNode endPtNode{gossip_target(infectedNode.id, time, n), time+1, const_cast<InfectedNode*>(&infectedNode)};
            //If the endpoint was already used, skip infecting it (note that target nodes are also on the used list)
            if (exclude_nodes.find(endPtNode.id) != exclude_nodes.end() && endPtNode.id != target)
                continue;
            //If we're reaching the target in less than the minimum time, skip infecting it
            if(endPtNode.id == target && (time - starting_round) < MIN_PATH_LENGTH)
                continue;
            //If we reached the target at the right time, return the path to it
            if(endPtNode.id == target) {
                list<int> path;
                path.push_back(endPtNode.id);
                //Construct the path backwards by following the parent pointers
                InfectedNode* parent = endPtNode.parent;
                while(parent != nullptr) {
                    path.push_front(parent->id);
                    parent = parent->parent;
                }
                return path;
            }
            //Otherwise, add the new infected node and continue
            newInfectedNodes.insert(std::move(endPtNode));
        }
        //Since InfectedNode's operator== is based only on id, this will not add nodes that were re-infected
        infected.insert(newInfectedNodes.begin(), newInfectedNodes.end());
    }
    throw std::runtime_error(std::string("Failed to find a path from ") + std::to_string(source) + std::string(" to ") + std::to_string(target));
}

} /* namespace util */
} /* namespace pddm */

