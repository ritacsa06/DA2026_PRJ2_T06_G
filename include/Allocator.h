#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <vector>
#include <stack>
#include <string>
#include <set>
#include <map>
#include "Graph.h"
#include "Web.h"

/**
 * @brief Result produced by the Allocator after running the coloring algorithm.
 */
struct AllocationResult {
    bool success = false;          ///< true if all webs were assigned a register
    int registersUsed = 0;         ///< number of distinct registers actually used
    std::vector<Web> webs;         ///< final webs with assignedRegister filled in
};

/**
 * @brief Performs register allocation through graph coloring.
 *
 * Implements the greedy simplification + coloring algorithm (T2.1 - basic):
 *
 *  Phase 1 – Simplification:
 *    Repeatedly remove nodes whose degree < K (the number of available
 *    registers) and push them onto a stack.  If all remaining nodes have
 *    degree >= K, one node is selected as a *spill candidate* (no register
 *    will be assigned to it) and removed so the loop can continue.
 *
 *  Phase 2 – Coloring:
 *    Pop nodes from the stack and assign the lowest-numbered color (register)
 *    not already used by any of its neighbours in the *original* graph.
 *    Because every pushed node had degree < K when it was removed, a valid
 *    color will always exist for it.  Spilled nodes receive NO_REGISTER.
 *
 * Time complexity: O(V^2 + E) per coloring attempt, where V = number of webs
 * and E = number of interference edges.
 */
class Allocator {
public:
    /**
     * @brief Constructs an Allocator for the given interference graph.
     * @param graph   The interference graph built by the Parser.
     * @param numRegs The maximum number of physical registers available (K).
     */
    Allocator(const Graph<Web>& graph, int numRegs);

    /**
     * @brief Runs the basic greedy graph-coloring register allocation (T2.1).
     *
     * Tries to color the interference graph with at most `numRegisters_` colors.
     * If the graph cannot be colored without spilling the allocation is still
     * attempted: spilled webs receive NO_REGISTER and success is set to false.
     *
     * @return AllocationResult with the coloring outcome.
     */
    AllocationResult allocate();

private:
    // ------------------------------------------------------------------ data
    const Graph<Web>& graph_;   ///< Original interference graph (read-only)
    int numRegisters_;          ///< K: maximum registers available

    // ------------------------------------------------------------------ helpers

    /**
     * @brief Returns the current effective degree of a vertex in the working
     *        graph, ignoring vertices that have already been removed/disabled.
     * @param v       The vertex to query.
     * @param removed Set of web ids that have been removed from the working graph.
     */
    int effectiveDegree(Vertex<Web>* v, const std::set<int>& removed) const;

    /**
     * @brief Selects the best spill candidate from the vertices still active
     *        in the working graph (those not in `removed`).
     *
     * Strategy: pick the node with the highest effective degree, as removing
     * it reduces interference the most and gives the best chance of coloring
     * the remainder of the graph.
     *
     * @param removed Set of web ids already removed from the working graph.
     * @return Pointer to the chosen spill-candidate vertex.
     */
    Vertex<Web>* chooseSpillCandidate(const std::set<int>& removed) const;

    /**
     * @brief Assigns the lowest available color (register index) to a vertex,
     *        considering the colors already used by its neighbours.
     * @param v       The vertex to color.
     * @param colors  Map from web id → assigned register (built incrementally).
     * @return The register index assigned, or NO_REGISTER if none was available.
     */
    int assignColor(Vertex<Web>* v, const std::map<int, int>& colors) const;
};

#endif // ALLOCATOR_H