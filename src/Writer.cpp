#include "Writer.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>


void Writer::write(const AllocationResult& result, const std::string& outputFile) {

    // Warn to console if allocation was not fully successful
    if (!result.success) {
        std::cerr << "\n[AVISO] A alocacao de registos nao foi possivel com o numero de registos fornecido.\n"
                  << "        Algumas webs foram enviadas para memoria (M).\n" << std::endl;
    }

    std::ofstream file(outputFile);
    if (!file.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o ficheiro de output: " + outputFile);
    }

    const std::vector<Web>& webs = result.webs;
    int numWebs = static_cast<int>(webs.size());

 
    file << "# Total number of webs followed by the listing of the program points of each one\n";
    file << "# program points in each web are sorted in ascending order\n";
    file << "webs: " << numWebs << "\n";

    for (int i = 0; i < numWebs; ++i) {
        file << "web" << i << ": " << formatWebPoints(webs[i]) << "\n";
    }

  
    file << "# Total number of registers used, followed by assignment to webs\n";
    file << "registers: " << result.registersUsed << "\n";

    // Group webs by register for the output lines (r0: webX, r0: webY, ...)
    // Registers first, then spilled webs
    std::map<int, std::vector<int>> regToWebs; // register → list of web indices
    std::vector<int> spilledWebIndices;

    for (int i = 0; i < numWebs; ++i) {
        if (webs[i].assignedRegister == NO_REGISTER) {
            spilledWebIndices.push_back(i);
        } else {
            regToWebs[webs[i].assignedRegister].push_back(i);
        }
    }

    // Print register assignments in order r0, r1, ...
    for (auto& [reg, webIndices] : regToWebs) {
        for (int idx : webIndices) {
            file << "r" << reg << ": web" << idx << "\n";
        }
    }

    // Print spilled webs
    for (int idx : spilledWebIndices) {
        file << "M: web" << idx << "\n";
    }

    file.close();

    std::cout << "[OK] Resultado escrito em: " << outputFile << std::endl;
}

std::string Writer::formatWebPoints(const Web& web) {
    std::ostringstream oss;
    bool first = true;

    // activeLines is already a sorted set
    for (int line : web.activeLines) {
        if (!first) oss << ",";
        first = false;

        oss << line;

        if (web.startLines.count(line)) {
            oss << "+";
        } else if (web.endLines.count(line)) {
            oss << "-";
        }
        // plain lines (neither start nor end) have no suffix
    }

    return oss.str();
}