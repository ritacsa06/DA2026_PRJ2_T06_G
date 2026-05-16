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
            // CORREÇÃO INTELECTUAL: Escolha condicional do candidato a spill
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

    // O motor apenas reporta os factos sem forçar o Tudo-Ou-Nada aqui
    result.success = (spillCount == 0);
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

// ---------------------------------------------------------------------------
// Public: allocateWithSplitting()  [T2.3 - splitting]
//
// Tenta colorir com o algoritmo básico. Se falhar, escolhe o web com maior
// grau (mais interferências) e divide-o em dois webs derivados, reconstruindo
// o grafo de interferências e tentando novamente. Repete até maxSplits vezes,
// parando logo que a coloração tenha sucesso (mínimo de splits usados).
//
// Racional da escolha: o web com maior grau é o que mais dificulta a coloração.
// O ponto de corte é escolhido para minimizar max(grau_esq, grau_dir) nos
// dois webs derivados, equilibrando a carga de interferências.
// ---------------------------------------------------------------------------

AllocationResult Allocator::allocateWithSplitting(int maxSplits) {

    if (graph_.getVertexSet().empty()) {
        AllocationResult r; r.success = true; return r;
    }

    // Cópia mutável dos webs para poder adicionar webs derivados
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

// ---------------------------------------------------------------------------
// Static: buildGraph() — constrói grafo de interferências a partir de webs
// ---------------------------------------------------------------------------
Graph<Web> Allocator::buildGraph(const std::vector<Web>& webs) {
    Graph<Web> g;
    for (const Web& w : webs) g.addVertex(w);
    for (size_t i = 0; i < webs.size(); ++i)
        for (size_t j = i + 1; j < webs.size(); ++j)
            if (websInterfere(webs[i], webs[j]))
                g.addBidirectionalEdge(webs[i], webs[j], 1.0);
    return g;
}

// ---------------------------------------------------------------------------
// Static: chooseSplitCandidate() — web com maior grau que tenha >= 2 linhas
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Static: splitWeb() — divide um web no ponto que minimiza max(deg_L, deg_R)
// ---------------------------------------------------------------------------
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
            if (websInterfere(left,  other)) ++dL;
            if (websInterfere(right, other)) ++dR;
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

bool Allocator::websInterfere(const Web& w1, const Web& w2) {
    for (int line : w1.activeLines) {
        if (!w2.activeLines.count(line)) continue;
        bool w1S = w1.startLines.count(line), w1E = w1.endLines.count(line);
        bool w2S = w2.startLines.count(line), w2E = w2.endLines.count(line);
        if ((w1S && w2E) || (w1E && w2S)) continue;
        return true;
    }
    return false;
}

Vertex<Web>* Allocator::chooseSmartSpillCandidate(const std::set<int>& removed) const {
    Vertex<Web>* best = nullptr;
    double bestScore = -1.0;

    for (Vertex<Web>* v : graph_.getVertexSet()) {
        int wid = v->getInfo().id;
        if (removed.count(wid)) continue;

        int deg = effectiveDegree(v, removed);
        
        // Quantas linhas de código esta variável ocupa?
        // Previne divisão por zero caso a web esteja estranhamente vazia
        int webSize = std::max(1, static_cast<int>(v->getInfo().activeLines.size())); 

        // A nossa heurística premium: Benefício (grau) a dividir pelo Custo (tamanho)
        double score = static_cast<double>(deg) / webSize;

        // Desempate: se o score for igual, escolhemos o id menor para determinismo
        if (score > bestScore || (score == bestScore && best != nullptr &&
                                  v->getInfo().id < best->getInfo().id)) {
            bestScore = score;
            best = v;
        }
    }
    return best;
}

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
        
        // Passamos 'true' no segundo argumento para ativar o modo inteligente!
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

