#include "Writer.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

void Writer::write(const AllocationResult& result, const std::string& outputFile) {
    
  
    AllocationResult finalResult = result;

   
    if (!finalResult.success) {
        std::cerr << "\n[AVISO] A alocacao de registos nao foi possivel com o numero de registos fornecido.\n"
                  << "        Todas as webs foram enviadas para memoria (M).\n" << std::endl;
    }

    std::ofstream file(outputFile);
    if (!file.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o ficheiro de output: " + outputFile);
    }

  
    const std::vector<Web>& webs = finalResult.webs;
    int numWebs = static_cast<int>(webs.size());

    file << "# Total number of webs followed by the listing of the program points of each one\n";
    file << "# program points in each web are sorted in ascending order\n";
    file << "webs: " << numWebs << "\n";

    for (int i = 0; i < numWebs; ++i) {
        file << "web" << i << ": " << formatWebPoints(webs[i]) << "\n";
    }

    file << "# Total number of registers used, followed by assignment to webs\n";
   
    file << "registers: " << finalResult.registersUsed << "\n";

    std::map<int, std::vector<int>> regToWebs;
    std::vector<int> spilledWebIndices;

    for (int i = 0; i < numWebs; ++i) {
        if (webs[i].assignedRegister == NO_REGISTER) {
            spilledWebIndices.push_back(i);
        } else {
            regToWebs[webs[i].assignedRegister].push_back(i);
        }
    }

    for (auto& [reg, webIndices] : regToWebs) {
        for (int idx : webIndices) {
            file << "r" << reg << ": web" << idx << "\n";
        }
    }

    for (int idx : spilledWebIndices) {
        file << "M: web" << idx << "\n";
    }

    file.close();

    std::cout << "[OK] Resultado escrito em: " << outputFile << std::endl;
}

std::string Writer::formatWebPoints(const Web& web) {
    std::ostringstream oss;
    bool first = true;

     for (int line : web.activeLines) {
        if (!first) oss << ",";
        first = false;

        oss << line;

        if (web.startLines.count(line)) {
            oss << "+";
        } else if (web.endLines.count(line)) {
            oss << "-";
        }
        
    }

    return oss.str();
}