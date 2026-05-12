#include "Allocator.h"
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <limits>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

Allocator::Allocator(const Graph<Web>& graph, int numRegs)
    : graph_(graph), numRegisters_(numRegs) {}

// ---------------------------------------------------------------------------
// Public: allocate()
// ---------------------------------------------------------------------------

AllocationResult Allocator::allocate() {
    AllocationResult result;

    // Collect all vertices from the original graph
    std::vector<Vertex<Web>*> allVertices = graph_.getVertexSet();

    if (allVertices.empty()) {
        result.success = true;
        result.registersUsed = 0;
        return result;
    }

    // -----------------------------------------------------------------------
    // Phase 1 – Simplification
    //
    // We simulate node removal using a "removed" set that tracks web ids of
    // vertices that have been taken out of the working graph.
    // Nodes with effective degree < K are pushed onto the coloring stack.
    // If we get stuck (every remaining node has degree >= K) we spill the
    // highest-degree node.
    // -----------------------------------------------------------------------

    std::set<int>       removed;       // web ids already pulled from working graph
    std::stack<int>     colorStack;    // web ids to be colored (in pop order)
    std::set<int>       spilledIds;    // web ids chosen for spilling

    int totalNodes = static_cast<int>(allVertices.size());

    while (static_cast<int>(removed.size()) < totalNodes) {

        // Try to find any node with effective degree < numRegisters_
        bool foundSimplifiable = false;

        for (Vertex<Web>* v : allVertices) {
            int wid = v->getInfo().id;
            if (removed.count(wid)) continue;

            if (effectiveDegree(v, removed) < numRegisters_) {
                removed.insert(wid);
                colorStack.push(wid);
                foundSimplifiable = true;
                // Restart the scan: removing this node may lower others' degrees
                break;
            }
        }

        if (!foundSimplifiable) {
            // Every remaining active node has degree >= K → must spill one
            Vertex<Web>* spillVertex = chooseSpillCandidate(removed);
            if (spillVertex == nullptr) break; // shouldn't happen

            int sid = spillVertex->getInfo().id;
            spilledIds.insert(sid);
            removed.insert(sid);
            // Do NOT push onto colorStack – spilled nodes get no color
        }
    }

    // -----------------------------------------------------------------------
    // Phase 2 – Coloring
    //
    // Pop nodes from the stack and greedily assign the lowest available color.
    // Each node had degree < K when it was simplified, so a color always exists.
    // -----------------------------------------------------------------------

    // colors: web id → register index (0-based)
    std::map<int, int> colors;

    // Pre-mark spilled nodes so neighbours know they have no color
    for (int sid : spilledIds) {
        colors[sid] = NO_REGISTER;
    }

    while (!colorStack.empty()) {
        int wid = colorStack.top();
        colorStack.pop();

        // Find the vertex with this id
        Vertex<Web>* v = nullptr;
        for (Vertex<Web>* candidate : allVertices) {
            if (candidate->getInfo().id == wid) { v = candidate; break; }
        }
        if (v == nullptr) continue;

        int reg = assignColor(v, colors);
        colors[wid] = reg;
    }

    // -----------------------------------------------------------------------
    // Build the result: copy webs from graph, fill in assigned registers
    // -----------------------------------------------------------------------

    int maxReg = -1;
    bool anySpill = false;

    for (Vertex<Web>* v : allVertices) {
        Web web = v->getInfo();
        auto it = colors.find(web.id);
        if (it != colors.end()) {
            web.assignedRegister = it->second;
        } else {
            web.assignedRegister = NO_REGISTER;
        }

        if (web.assignedRegister == NO_REGISTER) {
            anySpill = true;
        } else {
            maxReg = std::max(maxReg, web.assignedRegister);
        }

        result.webs.push_back(web);
    }

    // Sort webs by id for deterministic output
    std::sort(result.webs.begin(), result.webs.end(),
              [](const Web& a, const Web& b) { return a.id < b.id; });

    result.success       = !anySpill;
    result.registersUsed = (maxReg >= 0) ? (maxReg + 1) : 0;

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
        if (deg > bestDeg) {
            bestDeg = deg;
            best    = v;
        }
    }
    return best;
}

int Allocator::assignColor(Vertex<Web>* v, const std::map<int, int>& colors) const {
    // Collect colors already used by neighbours in the ORIGINAL graph
    std::set<int> usedColors;

    for (Edge<Web>* e : v->getAdj()) {
        int neighbourId = e->getDest()->getInfo().id;
        auto it = colors.find(neighbourId);
        if (it != colors.end() && it->second != NO_REGISTER) {
            usedColors.insert(it->second);
        }
    }

    // Return the lowest non-negative color not in usedColors
    for (int c = 0; c < numRegisters_; ++c) {
        if (!usedColors.count(c)) {
            return c;
        }
    }

    // This should not happen for simplified nodes, but guard defensively
    return NO_REGISTER;
}