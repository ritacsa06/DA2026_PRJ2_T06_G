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
    int websSpilled = 0;           ///< number of webs sent to memory
    std::vector<Web> webs;         ///< final webs with assignedRegister filled in
};

/**
 * @brief Performs register allocation through graph coloring.
 *
 * Supports two modes:
 *
 *  T2.1 – allocate():
 *    Basic greedy simplification + coloring with K colors.
 *    If the graph cannot be colored without spilling, spilled nodes receive
 *    NO_REGISTER and success is set to false.
 *
 *  T2.2 – allocateWithSpilling(maxSpills):
 *    Tries the basic algorithm first. If it fails, explicitly pre-spills the
 *    highest-degree web and retries. Repeats until coloring succeeds or
 *    maxSpills is exhausted.
 *
 *  T2.3 – allocateWithSplitting(maxSplits):
 *    Tries the basic algorithm first. If it fails, splits the highest-degree
 *    web into two derived webs, rebuilds the interference graph, and retries.
 *    Repeats until coloring succeeds or maxSplits is exhausted.
 *
 * Time complexity: O(V^2 + E) for basic; O(S * (V^2 + E)) for spilling/splitting
 * mode, where S = maxSpills/maxSplits, V = number of webs, E = interference edges.
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
     * Tries to color the interference graph with at most numRegisters_ colors.
     * If the graph cannot be colored without spilling, the allocation is still
     * attempted: spilled webs receive NO_REGISTER and success is set to false.
     *
     * @return AllocationResult with the coloring outcome.
     */
    AllocationResult allocate();

    /**
     * @brief Runs register allocation with controlled web spilling (T2.2).
     *
     * First attempts basic coloring with no forced spills. If it fails,
     * iteratively pre-spills the highest-degree web (maximum interference
     * removal per spill) and retries, up to maxSpills times total.
     * Returns as soon as coloring succeeds (minimum spills used).
     * If maxSpills is exhausted, returns the last attempted result.
     *
     * Rationale for highest-degree selection: removing the most connected node
     * reduces the maximum clique size the most, giving the best chance of
     * making the remaining graph K-colorable with a single spill.
     *
     * @param maxSpills Maximum number of webs allowed to be pre-spilled.
     * @return AllocationResult with the coloring outcome.
     */
    AllocationResult allocateWithSpilling(int maxSpills);

    AllocationResult allocateWithSplitting(int maxSplits);
        /**
     * @brief Executa a alocacao livre otimizada por Custo-Beneficio (T2.4).
     * Escolhe candidatos a spill baseando-se no racio Grau / Tamanho da Web.
     */
    AllocationResult allocateFree();

private:
    // ------------------------------------------------------------------ data
    const Graph<Web>& graph_;   ///< Original interference graph (read-only)
    int numRegisters_;          ///< K: maximum registers available

    // ------------------------------------------------------------------ helpers

    /**
     * @brief Core coloring engine used by both public methods.
     *
     * Runs the simplification + coloring loop. Webs in forcedSpills are
     * removed from the working graph before simplification begins so they
     * never consume a register slot. Any additional webs that cannot be
     * simplified without spilling are also marked NO_REGISTER.
     *
     * @param forcedSpills Set of web ids that must be spilled regardless.
     * @return AllocationResult with the coloring outcome.
     */
    AllocationResult runColoring(const std::set<int>& forcedSpills, bool useSmartSpill = false) const;

    /**
     * @brief Returns the effective degree of a vertex in the working graph,
     *        ignoring vertices that have already been removed.
     * @param v       The vertex to query.
     * @param removed Set of web ids removed from the working graph.
     */
    int effectiveDegree(Vertex<Web>* v, const std::set<int>& removed) const;

    /**
     * @brief Selects the best spill candidate among active vertices.
     *
     * Picks the node with the highest effective degree. Ties broken by web id
     * for determinism.
     *
     * @param removed Set of web ids already removed from the working graph.
     * @return Pointer to the chosen vertex, or nullptr if none remain.
     */
    Vertex<Web>* chooseSpillCandidate(const std::set<int>& removed) const;
    Vertex<Web>* chooseSmartSpillCandidate(const std::set<int>& removed) const;
    /**
     * @brief Assigns the lowest available color to a vertex.
     *
     * Scans neighbour colors in the original graph and returns the first
     * color in [0, numRegisters_) not already used by a neighbour.
     *
     * @param v       The vertex to color.
     * @param colors  Map from web id to assigned register (built incrementally).
     * @return The register index assigned, or NO_REGISTER if none available.
     */
    int assignColor(Vertex<Web>* v, const std::map<int, int>& colors) const;

    /**
     * @brief Builds an interference graph from a given list of webs.
     *
     * Used by allocateWithSplitting to rebuild the graph after each split.
     * Two webs interfere if they share at least one active line that is not
     * a definition-vs-last-use boundary.
     *
     * @param webs The webs to put in the graph.
     * @return A new Graph<Web> with the interference edges.
     */
    static Graph<Web> buildGraph(const std::vector<Web>& webs);

    /**
     * @brief Selects the web to split: the one with the highest degree.
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
     * Tries every possible cut point (between consecutive active lines) and
     * picks the one that minimises max(degree_left, degree_right) in the
     * current interference graph. Ties broken by choosing the middle cut.
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