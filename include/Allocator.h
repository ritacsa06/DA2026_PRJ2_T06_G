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
    bool success = false;          ///< True if all webs were successfully assigned a register.
    int registersUsed = 0;         ///< Number of distinct registers actually used.
    int websSpilled = 0;           ///< Number of webs sent to memory.
    std::vector<Web> webs;         ///< Final webs with the assignedRegister field filled in.
};

/**
 * @brief Performs register allocation through graph coloring.
 *
 * @details Supports four modes corresponding to the project tasks:
 * - T2.1 (allocate): Basic greedy simplification + coloring.
 * - T2.2 (allocateWithSpilling): Coloring with up to K controlled web spills.
 * - T2.3 (allocateWithSplitting): Coloring with up to K web splits.
 * - T2.4 (allocateFree): Custom heuristic using a Cost-Benefit metric.
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
     * @details Tries to color the interference graph with at most numRegisters_ colors.
     * If the graph cannot be colored without spilling, the allocation is still
     * attempted: spilled webs receive NO_REGISTER and success is set to false.
     * * <b>Time Complexity:</b> O(V^2 + E), where V is the number of webs and E is the number of interference edges.
     *
     * @return AllocationResult with the coloring outcome.
     */
    AllocationResult allocate();

    /**
     * @brief Runs register allocation with controlled web spilling (T2.2).
     *
     * @details First attempts basic coloring with no forced spills. If it fails,
     * iteratively pre-spills the highest-degree web (maximum interference
     * removal per spill) and retries, up to maxSpills times total.
     * Returns as soon as coloring succeeds (minimum spills used).
     * * <b>Time Complexity:</b> O(S * (V^2 + E)), where S is maxSpills, V is the number of webs, and E is the number of edges.
     *
     * @param maxSpills Maximum number of webs allowed to be pre-spilled.
     * @return AllocationResult with the coloring outcome.
     */
    AllocationResult allocateWithSpilling(int maxSpills);

    /**
     * @brief Runs register allocation with controlled web splitting (T2.3).
     *
     * @details Attempts basic coloring. If it fails, splits the highest-degree web into 
     * two derived webs, rebuilds the interference graph, and retries.
     * Repeats until coloring succeeds or maxSplits is exhausted.
     * * <b>Time Complexity:</b> O(S * V^2 * L), where S is maxSplits, V is the number of webs, and L is the maximum number of active lines in a web.
     * * @param maxSplits Maximum number of webs allowed to be split.
     * @return AllocationResult with the coloring outcome.
     */
    AllocationResult allocateWithSplitting(int maxSplits);
    
    /**
     * @brief Executes the custom Cost-Benefit register allocation algorithm (T2.4).
     * * @details Uses a smart spill candidate selection based on the ratio of the 
     * web's effective degree to its size (number of active lines). This minimizes 
     * memory access overhead by keeping long-living variables in registers.
     * * <b>Time Complexity:</b> O(V * (V^2 + E)) in the worst case, as it may attempt to spill up to V webs.
     * * @return AllocationResult with the coloring outcome.
     */
    AllocationResult allocateFree();

private:
    // ------------------------------------------------------------------ data
    const Graph<Web>& graph_;   ///< Original interference graph (read-only)
    int numRegisters_;          ///< K: maximum registers available

    // ------------------------------------------------------------------ helpers

    /**
     * @brief Core coloring engine used by the allocation methods.
     *
     * @details Runs the simplification and coloring loop. Webs in forcedSpills are
     * removed from the working graph before simplification begins so they
     * never consume a register slot.
     * * <b>Time Complexity:</b> O(V^2 + E), where V is the number of webs and E is the number of edges.
     *
     * @param forcedSpills Set of web ids that must be spilled regardless.
     * @param useSmartSpill If true, uses the T2.4 Cost-Benefit heuristic for spills.
     * @return AllocationResult with the coloring outcome.
     */
    AllocationResult runColoring(const std::set<int>& forcedSpills, bool useSmartSpill = false) const;

    /**
     * @brief Returns the effective degree of a vertex in the working graph.
     * * @details Ignores vertices that have already been removed (spilled or simplified).
     * * <b>Time Complexity:</b> O(D), where D is the degree of the vertex.
     * * @param v       The vertex to query.
     * @param removed Set of web ids removed from the working graph.
     * @return The effective degree of the vertex.
     */
    int effectiveDegree(Vertex<Web>* v, const std::set<int>& removed) const;

    /**
     * @brief Selects the best spill candidate among active vertices (T2.2).
     *
     * @details Picks the node with the highest effective degree. Ties broken by web id.
     * * <b>Time Complexity:</b> O(V + E), where V is the number of vertices and E is the number of edges.
     *
     * @param removed Set of web ids already removed from the working graph.
     * @return Pointer to the chosen vertex, or nullptr if none remain.
     */
    Vertex<Web>* chooseSpillCandidate(const std::set<int>& removed) const;
    
    /**
     * @brief Selects the smartest spill candidate based on Cost-Benefit (T2.4).
     * * @details Calculates a score = (Effective Degree) / (Web Size). The node with the highest score is chosen.
     * * <b>Time Complexity:</b> O(V + E).
     * * @param removed Set of web ids already removed from the working graph.
     * @return Pointer to the chosen vertex, or nullptr if none remain.
     */
    Vertex<Web>* chooseSmartSpillCandidate(const std::set<int>& removed) const;

    /**
     * @brief Assigns the lowest available color to a vertex.
     *
     * @details Scans neighbour colors in the original graph and returns the first
     * color in [0, numRegisters_) not already used by a neighbour.
     * * <b>Time Complexity:</b> O(D + K), where D is the degree of the vertex and K is the number of registers.
     *
     * @param v       The vertex to color.
     * @param colors  Map from web id to assigned register (built incrementally).
     * @return The register index assigned, or NO_REGISTER if none available.
     */
    int assignColor(Vertex<Web>* v, const std::map<int, int>& colors) const;

    /**
     * @brief Builds an interference graph from a given list of webs.
     *
     * @details Used by allocateWithSplitting to rebuild the graph after each split.
     * * <b>Time Complexity:</b> O(V^2 * L), where V is the number of webs and L is the maximum active lines per web.
     *
     * @param webs The webs to put in the graph.
     * @return A new Graph<Web> with the interference edges.
     */
    static Graph<Web> buildGraph(const std::vector<Web>& webs);

    /**
     * @brief Selects the web to split: the one with the highest degree (T2.3).
     * * <b>Time Complexity:</b> O(V), where V is the number of webs.
     *
     * @param webs    Current web list.
     * @param graph   Current interference graph.
     * @return Index into `webs` of the chosen web, or -1 if none splittable.
     */
    static int chooseSplitCandidate(const std::vector<Web>& webs,
                                    const Graph<Web>& graph);

    /**
     * @brief Splits a web into two derived webs at the best cut point.
     *
     * @details Tries every possible cut point (between consecutive active lines) and
     * picks the one that minimises max(degree_left, degree_right) in the
     * current interference graph.
     * * <b>Time Complexity:</b> O(L * V), where L is the number of active lines in the web and V is the total number of webs.
     *
     * @param web       The web to split.
     * @param allWebs   All current webs (used to compute interference).
     * @param nextId    Next available web id (incremented for the new web).
     * @return Pair {left_web, right_web}.
     */
    static std::pair<Web, Web> splitWeb(const Web& web,
                                        const std::vector<Web>& allWebs,
                                        int& nextId);
};

#endif // ALLOCATOR_H