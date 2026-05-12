#include "Allocator.h"
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <climits>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

Allocator::Allocator(const Graph<Web>& graph, int numRegs)
    : graph_(graph), numRegisters_(numRegs) {}

// ---------------------------------------------------------------------------
// Public: allocate()  [T2.1 - basic]
// Delegates directly to runColoring with no forced spills.
// ---------------------------------------------------------------------------

AllocationResult Allocator::allocate() {
    return runColoring({});
}

// ---------------------------------------------------------------------------
// Public: allocateWithSpilling()  [T2.2 - spilling]
//
// Strategy:
//   Start with no forced spills. If coloring fails (the basic algorithm
//   was forced to spill internally), explicitly pre-spill the highest-degree
//   web and retry. Repeat until success or maxSpills exhausted.
//
//   We always pick the highest-degree web as the next spill candidate because
//   it carries the most interference edges. Removing it from the graph reduces
//   the maximum clique size the most, giving the remaining webs the best
//   chance of being K-colorable with one fewer node.
//
//   We track the "best" result seen so far (fewest internal spills) so that
//   if we exhaust maxSpills without a clean success we still return something
//   meaningful.
// ---------------------------------------------------------------------------

AllocationResult Allocator::allocateWithSpilling(int maxSpills) {

    // Guard: if graph is empty just return success immediately
    if (graph_.getVertexSet().empty()) {
        AllocationResult r;
        r.success = true;
        return r;
    }

    std::set<int>    forcedSpills;   // web ids pre-committed to memory
    AllocationResult bestResult;     // best result seen across all attempts
    bestResult.websSpilled = INT_MAX;

    for (int attempt = 0; attempt <= maxSpills; ++attempt) {

        AllocationResult result = runColoring(forcedSpills);

        // Track best (fewest total spills)
        if (result.websSpilled < bestResult.websSpilled) {
            bestResult = result;
        }

        // Success: coloring worked with no unexpected spills — stop early
        if (result.websSpilled == static_cast<int>(forcedSpills.size())) {
            std::cout << "    [Spilling] Coloracao bem-sucedida com "
                      << static_cast<int>(forcedSpills.size())
                      << " web(s) derramada(s) para memoria." << std::endl;
            return result;
        }

        // Coloring failed — choose the next web to pre-spill
        if (attempt < maxSpills) {
            // Build removed set = currently forced spills (already out of graph)
            Vertex<Web>* candidate = chooseSpillCandidate(forcedSpills);
            if (candidate == nullptr) break; // no more nodes to spill

            int cid = candidate->getInfo().id;
            forcedSpills.insert(cid);

            std::cout << "    [Spilling] Tentativa " << (attempt + 1)
                      << ": a derramar web id=" << cid
                      << " (variavel: " << candidate->getInfo().variableName
                      << ", grau=" << effectiveDegree(candidate, forcedSpills) + 1
                      << ") e a tentar novamente..." << std::endl;
        }
    }

    // Exhausted maxSpills — return best result found
    std::cerr << "\n[AVISO] Nao foi possivel colorir o grafo com "
              << numRegisters_ << " registos e no maximo "
              << maxSpills << " web(s) derramada(s).\n"
              << "        " << bestResult.websSpilled
              << " web(s) enviada(s) para memoria no melhor resultado.\n";

    return bestResult;
}

// ---------------------------------------------------------------------------
// Private: runColoring()
//
// Core engine used by both public methods.
//
// Phase 1 – Simplification:
//   forcedSpills are removed immediately (they never compete for a register).
//   Then we repeatedly remove nodes with effective degree < K onto a stack.
//   If stuck (all remaining have degree >= K) we pick another spill candidate
//   — this is an "unplanned" spill that counts against success.
//
// Phase 2 – Coloring:
//   Pop the stack; assign the lowest color not used by any neighbour in the
//   original graph. Forced/unplanned spills both receive NO_REGISTER.
// ---------------------------------------------------------------------------

AllocationResult Allocator::runColoring(const std::set<int>& forcedSpills) const {

    AllocationResult result;
    std::vector<Vertex<Web>*> allVertices = graph_.getVertexSet();

    if (allVertices.empty()) {
        result.success = true;
        return result;
    }

    // "removed" tracks everything out of the working graph:
    // starts with forcedSpills, grows as nodes are simplified or spilled.
    std::set<int>   removed     = forcedSpills;  // already out of working graph
    std::set<int>   spilledIds  = forcedSpills;  // all ids that get NO_REGISTER
    std::stack<int> colorStack;                  // ids to color (in pop order)

    int totalNodes = static_cast<int>(allVertices.size());

    // -----------------------------------------------------------------------
    // Phase 1 – Simplification
    // -----------------------------------------------------------------------
    while (static_cast<int>(removed.size()) < totalNodes) {

        bool foundSimplifiable = false;

        for (Vertex<Web>* v : allVertices) {
            int wid = v->getInfo().id;
            if (removed.count(wid)) continue;

            if (effectiveDegree(v, removed) < numRegisters_) {
                removed.insert(wid);
                colorStack.push(wid);
                foundSimplifiable = true;
                break; // restart scan — degrees may have changed
            }
        }

        if (!foundSimplifiable) {
            // All remaining nodes have degree >= K: unplanned spill
            Vertex<Web>* spillVertex = chooseSpillCandidate(removed);
            if (spillVertex == nullptr) break;

            int sid = spillVertex->getInfo().id;
            spilledIds.insert(sid);
            removed.insert(sid);
            // Not pushed onto colorStack — no register assigned
        }
    }

    // -----------------------------------------------------------------------
    // Phase 2 – Coloring
    // -----------------------------------------------------------------------
    std::map<int, int> colors;

    // Pre-mark all spilled nodes (forced + unplanned)
    for (int sid : spilledIds) {
        colors[sid] = NO_REGISTER;
    }

    while (!colorStack.empty()) {
        int wid = colorStack.top();
        colorStack.pop();

        Vertex<Web>* v = nullptr;
        for (Vertex<Web>* candidate : allVertices) {
            if (candidate->getInfo().id == wid) { v = candidate; break; }
        }
        if (v == nullptr) continue;

        colors[wid] = assignColor(v, colors);
    }

    // -----------------------------------------------------------------------
    // Build AllocationResult
    // -----------------------------------------------------------------------
    int  maxReg    = -1;
    int  spillCount = 0;

    for (Vertex<Web>* v : allVertices) {
        Web web = v->getInfo();
        auto it = colors.find(web.id);
        web.assignedRegister = (it != colors.end()) ? it->second : NO_REGISTER;

        if (web.assignedRegister == NO_REGISTER) {
            ++spillCount;
        } else {
            maxReg = std::max(maxReg, web.assignedRegister);
        }

        result.webs.push_back(web);
    }

    std::sort(result.webs.begin(), result.webs.end(),
              [](const Web& a, const Web& b) { return a.id < b.id; });

    result.success       = (spillCount == 0);
    result.registersUsed = (maxReg >= 0) ? (maxReg + 1) : 0;
    result.websSpilled   = spillCount;

    return result;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int Allocator::effectiveDegree(Vertex<Web>* v, const std::set<int>& removed) const {
    int degree = 0;
    for (Edge<Web>* e : v->getAdj()) {
        int neighbourId = e->getDest()->getInfo().id;
        if (!removed.count(neighbourId)) {
            ++degree;
        }
    }
    return degree;
}

Vertex<Web>* Allocator::chooseSpillCandidate(const std::set<int>& removed) const {
    Vertex<Web>* best    = nullptr;
    int          bestDeg = -1;

    for (Vertex<Web>* v : graph_.getVertexSet()) {
        int wid = v->getInfo().id;
        if (removed.count(wid)) continue;

        int deg = effectiveDegree(v, removed);
        // Tie-break by web id for determinism
        if (deg > bestDeg || (deg == bestDeg && best != nullptr &&
                              v->getInfo().id < best->getInfo().id)) {
            bestDeg = deg;
            best    = v;
        }
    }
    return best;
}

int Allocator::assignColor(Vertex<Web>* v, const std::map<int, int>& colors) const {
    std::set<int> usedColors;

    for (Edge<Web>* e : v->getAdj()) {
        int neighbourId = e->getDest()->getInfo().id;
        auto it = colors.find(neighbourId);
        if (it != colors.end() && it->second != NO_REGISTER) {
            usedColors.insert(it->second);
        }
    }

    for (int c = 0; c < numRegisters_; ++c) {
        if (!usedColors.count(c)) {
            return c;
        }
    }

    // Defensive fallback — should not occur for properly simplified nodes
    return NO_REGISTER;
}