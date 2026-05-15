#include "Allocator.h"
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <climits>

Allocator::Allocator(const Graph<Web>& graph, int numRegs)
    : graph_(graph), numRegisters_(numRegs) {}

AllocationResult Allocator::allocate() {
    return runColoring({});
}

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
                      << ", grau=" << effectiveDegree(candidate, forcedSpills) + 1
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

AllocationResult Allocator::runColoring(const std::set<int>& forcedSpills) const {

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
            Vertex<Web>* spillVertex = chooseSpillCandidate(removed);
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

    result.success       = (spillCount == 0);
    result.registersUsed = (maxReg >= 0) ? (maxReg + 1) : 0;
    result.websSpilled   = spillCount;

    return result;
}


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

    return NO_REGISTER;
}

AllocationResult Allocator::runColoring(const std::set<int>& forcedSpills) const {
   AllocationResult result;
   result.webs = std::vector<Web>();
  
   std::map<int, int> colors;
   std::stack<Vertex<Web>*> s;
   std::set<int> removed;


   for (int id : forcedSpills) {
       removed.insert(id);
       colors[id] = NO_REGISTER;
   }


   while (removed.size() < graph_.getVertexSet().size()) {
       Vertex<Web>* candidate = nullptr;


       for (auto v : graph_.getVertexSet()) {
           int wid = v->getInfo().id;
           if (removed.count(wid)) continue;


           if (effectiveDegree(v, removed) < numRegisters_) {
               candidate = v;
               break;
           }
       }


       if (candidate == nullptr) {
           candidate = chooseSpillCandidate(removed);
       }


       if (candidate) {
           s.push(candidate);
           removed.insert(candidate->getInfo().id);
       }
   }


   while (!s.empty()) {
       Vertex<Web>* v = s.top();
       s.pop();


       int reg = assignColor(v, colors);
       colors[v->getInfo().id] = reg;


       if (reg == NO_REGISTER) {
           result.websSpilled++;
       }
   }


   result.success = (result.websSpilled == 0);
   std::set<int> uniqueRegs;
  
   for (auto v : graph_.getVertexSet()) {
       Web w = v->getInfo();
       w.assignedRegister = colors[w.id];
       result.webs.push_back(w);
       if (w.assignedRegister != NO_REGISTER) {
           uniqueRegs.insert(w.assignedRegister);
       }
   }
   result.registersUsed = uniqueRegs.size();


   return result;
}
