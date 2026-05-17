/**
 * @file Allocator.cpp
 * @brief Implementation of the Allocator class algorithms for register allocation.
 */

#include "Allocator.h"
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <climits>

/**
 * @brief Constructs an Allocator.
 * <b>Time Complexity:</b> O(1).
 */
Allocator::Allocator(const Graph<Web>& graph, int numRegs)
    : graph_(graph), numRegisters_(numRegs) {}

/**
 * @brief Executes basic allocation without any controlled spilling or splitting.
 * <b>Time Complexity:</b> O(V^2 + E), where V is the number of webs and E is the number of edges.
 */
AllocationResult Allocator::allocate() {
    return runColoring({});
}

/**
 * @brief Executes allocation allowing up to maxSpills controlled spills.
 * @details Iteratively removes the web with the highest effective degree and retries coloring.
 * <b>Time Complexity:</b> O(S * (V^2 + E)), where S is maxSpills.
 */
AllocationResult Allocator::allocateWithSpilling(int maxSpills) {
    if (graph_.getVertexSet().empty()) {
        AllocationResult r;
        r.success = true;
        return r;
    }

    std::set<int>    forcedSpills;   
    AllocationResult bestResult;     
    bestResult.websSpilled = INT_MAX;

    for (int attempt = 0; attempt <= maxSpills; ++attempt) {
        AllocationResult result = runColoring(forcedSpills);

        if (result.websSpilled < bestResult.websSpilled) {
            bestResult = result;
        }

        if (result.websSpilled == static_cast<int>(forcedSpills.size())) {
            std::cout << "    [Spilling] Coloracao bem-sucedida com "
                      << static_cast<int>(forcedSpills.size())
                      << " web(s) derramada(s) para memoria." << std::endl;
            result.success = true;
            return result;
        }

        if (attempt < maxSpills) {
            Vertex<Web>* candidate = chooseSpillCandidate(forcedSpills);
            if (candidate == nullptr) break;

            int cid = candidate->getInfo().id;
            forcedSpills.insert(cid);

            std::cout << "    [Spilling] Tentativa " << (attempt + 1)
                      << ": a derramar web id=" << cid
                      << " (variavel: " << candidate->getInfo().variableName
                      << ", grau=" << effectiveDegree(candidate, forcedSpills)
                      << ") e a tentar novamente..." << std::endl;
        }
    }

    std::cerr << "\n[AVISO] Nao foi possivel colorir o grafo com "
              << numRegisters_ << " registos e no maximo "
              << maxSpills << " web(s) derramada(s).\n"
              << "        " << bestResult.websSpilled
              << " web(s) enviada(s) para memoria no melhor resultado.\n";

    return bestResult;
}

/**
 * @brief Core engine that performs graph simplification and coloring.
 * @details Pushes nodes with degree < K onto a stack. Nodes remaining are spilled.
 * Then pops nodes and assigns the lowest available color.
 * <b>Time Complexity:</b> O(V^2 + E), as it scans vertices and their adjacencies.
 */
AllocationResult Allocator::runColoring(const std::set<int>& forcedSpills, bool useSmartSpill) const {
    AllocationResult result;
    std::vector<Vertex<Web>*> allVertices = graph_.getVertexSet();

    if (allVertices.empty()) {
        result.success = true;
        return result;
    }

    std::set<int>   removed     = forcedSpills;  
    std::set<int>   spilledIds  = forcedSpills;  
    std::stack<int> colorStack;                 

    int totalNodes = static_cast<int>(allVertices.size());

    while (static_cast<int>(removed.size()) < totalNodes) {
        bool foundSimplifiable = false;

        for (Vertex<Web>* v : allVertices) {
            int wid = v->getInfo().id;
            if (removed.count(wid)) continue;

            if (effectiveDegree(v, removed) < numRegisters_) {
                removed.insert(wid);
                colorStack.push(wid);
                foundSimplifiable = true;
                break; 
            }
        }

        if (!foundSimplifiable) {
            Vertex<Web>* spillVertex = useSmartSpill ? chooseSmartSpillCandidate(removed) 
                                                     : chooseSpillCandidate(removed);
            if (spillVertex == nullptr) break;

            int sid = spillVertex->getInfo().id;
            spilledIds.insert(sid);
            removed.insert(sid);
        }
    }

    std::map<int, int> colors;
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

    result.success = (spillCount == 0);
    result.registersUsed = (maxReg >= 0) ? (maxReg + 1) : 0;
    result.websSpilled   = spillCount;

    return result;
}

/**
 * @brief Calculates the effective degree of a vertex excluding removed neighbors.
 * <b>Time Complexity:</b> O(D), where D is the out-degree of the vertex.
 */
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

/**
 * @brief Selects the standard spill candidate based purely on highest degree.
 * <b>Time Complexity:</b> O(V + E) to calculate effective degrees for all active nodes.
 */
Vertex<Web>* Allocator::chooseSpillCandidate(const std::set<int>& removed) const {
    Vertex<Web>* best    = nullptr;
    int          bestDeg = -1;

    for (Vertex<Web>* v : graph_.getVertexSet()) {
        int wid = v->getInfo().id;
        if (removed.count(wid)) continue;

        int deg = effectiveDegree(v, removed);
        if (deg > bestDeg || (deg == bestDeg && best != nullptr &&
                              v->getInfo().id < best->getInfo().id)) {
            bestDeg = deg;
            best    = v;
        }
    }
    return best;
}

/**
 * @brief Finds the lowest available color not used by neighboring vertices.
 * <b>Time Complexity:</b> O(D + K), where D is degree and K is numRegisters.
 */
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

    return NO_REGISTER;
}

/**
 * @brief Executes allocation allowing up to maxSplits controlled web splits.
 * <b>Time Complexity:</b> O(S * V^2 * L), where S is maxSplits.
 */
AllocationResult Allocator::allocateWithSplitting(int maxSplits) {
    if (graph_.getVertexSet().empty()) {
        AllocationResult r; r.success = true; return r;
    }

    std::vector<Web> currentWebs;
    for (Vertex<Web>* v : graph_.getVertexSet()) {
        currentWebs.push_back(v->getInfo());
    }

    int nextId = 0;
    for (const Web& w : currentWebs) nextId = std::max(nextId, w.id + 1);

    AllocationResult bestResult;
    bestResult.websSpilled = INT_MAX;

    for (int attempt = 0; attempt <= maxSplits; ++attempt) {
        Graph<Web> currentGraph = buildGraph(currentWebs);
        Allocator tempAllocator(currentGraph, numRegisters_);
        AllocationResult result = tempAllocator.runColoring({});

        if (result.websSpilled < bestResult.websSpilled) bestResult = result;

        if (result.success) {
            std::cout << "    [Splitting] Coloracao bem-sucedida com "
                      << attempt << " split(s) efectuado(s)." << std::endl;
            return result;
        }

        if (attempt < maxSplits) {
            int idx = chooseSplitCandidate(currentWebs, currentGraph);
            if (idx < 0) break;

            const Web& chosen = currentWebs[idx];
            std::cout << "    [Splitting] Split " << (attempt + 1)
                      << ": a dividir web id=" << chosen.id
                      << " (variavel: " << chosen.variableName
                      << ", linhas=" << chosen.activeLines.size() << ")" << std::endl;

            auto [left, right] = splitWeb(chosen, currentWebs, nextId);
            currentWebs.erase(currentWebs.begin() + idx);
            currentWebs.push_back(left);
            currentWebs.push_back(right);
        }
    }

    std::cerr << "\n[AVISO] Nao foi possivel colorir o grafo com "
              << numRegisters_ << " registos e no maximo "
              << maxSplits << " split(s).\n"
              << "        " << bestResult.websSpilled
              << " web(s) enviada(s) para memoria no melhor resultado.\n";
    return bestResult;
}

/**
 * @brief Constructs a new interference graph from a vector of webs.
 * <b>Time Complexity:</b> O(V^2 * L), checks interference for all pairs.
 */
Graph<Web> Allocator::buildGraph(const std::vector<Web>& webs) {
    Graph<Web> g;
    for (const Web& w : webs) g.addVertex(w);
    for (size_t i = 0; i < webs.size(); ++i)
        for (size_t j = i + 1; j < webs.size(); ++j)
            if (websInterfereGlobal(webs[i], webs[j]))
                g.addBidirectionalEdge(webs[i], webs[j], 1.0);
    return g;
}

/**
 * @brief Selects a web to be split based on the highest degree and size >= 2.
 * <b>Time Complexity:</b> O(V).
 */
int Allocator::chooseSplitCandidate(const std::vector<Web>& webs,
                                     const Graph<Web>& graph) {
    int bestIdx = -1, bestDeg = -1;
    for (size_t i = 0; i < webs.size(); ++i) {
        if ((int)webs[i].activeLines.size() < 2) continue;
        Vertex<Web>* v = graph.findVertex(webs[i]);
        if (!v) continue;
        int deg = (int)v->getAdj().size();
        if (deg > bestDeg) { bestDeg = deg; bestIdx = (int)i; }
    }
    return bestIdx;
}

/**
 * @brief Divides a web into two at the optimal cut point to minimize resulting interferences.
 * <b>Time Complexity:</b> O(L^2 * V), tests all possible cut points L against all other webs V.
 */
std::pair<Web, Web> Allocator::splitWeb(const Web& web,
                                         const std::vector<Web>& allWebs,
                                         int& nextId) {
    std::vector<int> lines(web.activeLines.begin(), web.activeLines.end());
    int n = (int)lines.size();
    int bestCut = n / 2, bestScore = INT_MAX;

    for (int cut = 1; cut < n; ++cut) {
        Web left, right;
        for (int k = 0;   k < cut; ++k) left.activeLines.insert(lines[k]);
        for (int k = cut; k < n;   ++k) right.activeLines.insert(lines[k]);
        for (int x : web.startLines) {
            if (left.activeLines.count(x))  left.startLines.insert(x);
            else                             right.startLines.insert(x);
        }
        for (int x : web.endLines) {
            if (left.activeLines.count(x))  left.endLines.insert(x);
            else                             right.endLines.insert(x);
        }
        int dL = 0, dR = 0;
        for (const Web& other : allWebs) {
            if (other.id == web.id) continue;
            if (websInterfereGlobal(left,  other)) ++dL;
            if (websInterfereGlobal(right, other)) ++dR;
        }
        int score = std::max(dL, dR);
        if (score < bestScore) { bestScore = score; bestCut = cut; }
    }

    Web left, right;
    left.id  = web.id; right.id = nextId++;
    left.variableName = right.variableName = web.variableName;
    for (int k = 0;       k < bestCut; ++k) left.activeLines.insert(lines[k]);
    for (int k = bestCut; k < n;       ++k) right.activeLines.insert(lines[k]);
    for (int x : web.startLines) {
        if (left.activeLines.count(x))  left.startLines.insert(x);
        else                             right.startLines.insert(x);
    }
    for (int x : web.endLines) {
        if (left.activeLines.count(x))  left.endLines.insert(x);
        else                             right.endLines.insert(x);
    }
    return {left, right};
}

/**
 * @brief Selects a spill candidate using the Cost-Benefit ratio (Degree / Size).
 * <b>Time Complexity:</b> O(V + E) to calculate scores for active nodes.
 */
Vertex<Web>* Allocator::chooseSmartSpillCandidate(const std::set<int>& removed) const {
    Vertex<Web>* best = nullptr;
    double bestScore = -1.0;

    for (Vertex<Web>* v : graph_.getVertexSet()) {
        int wid = v->getInfo().id;
        if (removed.count(wid)) continue;

        int deg = effectiveDegree(v, removed);
        int webSize = std::max(1, static_cast<int>(v->getInfo().activeLines.size())); 

        double score = static_cast<double>(deg) / webSize;

        if (score > bestScore || (score == bestScore && best != nullptr &&
                                  v->getInfo().id < best->getInfo().id)) {
            bestScore = score;
            best = v;
        }
    }
    return best;
}

/**
 * @brief Executes allocation using the custom Cost-Benefit smart heuristic.
 * <b>Time Complexity:</b> O(V * (V^2 + E)), as it iterates through possible smart spills.
 */
AllocationResult Allocator::allocateFree() {
    std::vector<Vertex<Web>*> allVertices = graph_.getVertexSet();
    if (allVertices.empty()) {
        AllocationResult r; r.success = true; return r;
    }

    std::set<int> forcedSpills;
    AllocationResult bestResult;
    bestResult.websSpilled = INT_MAX;
    
    int maxPossibleSpills = static_cast<int>(allVertices.size());

    for (int attempt = 0; attempt <= maxPossibleSpills; ++attempt) {
        AllocationResult result = runColoring(forcedSpills, true);

        if (result.websSpilled < bestResult.websSpilled) {
            bestResult = result;
        }

        if (result.success || result.websSpilled == static_cast<int>(forcedSpills.size())) {
            std::cout << "    [Modo Livre] Alocacao finalizada com "
                      << static_cast<int>(forcedSpills.size())
                      << " web(s) derramada(s) de forma inteligente." << std::endl;
            result.success = true;
            return result;
        }

        if (attempt < maxPossibleSpills) {
            Vertex<Web>* candidate = chooseSmartSpillCandidate(forcedSpills);
            if (candidate == nullptr) break;

            int cid = candidate->getInfo().id;
            forcedSpills.insert(cid);

            std::cout << "    [Modo Livre] Tentativa " << (attempt + 1)
                      << ": Spill inteligente da web id=" << cid
                      << " (Variavel: " << candidate->getInfo().variableName
                      << ", Racio Custo-Beneficio favoravel) e a tentar novamente..." << std::endl;
        }
    }

    return bestResult;
}