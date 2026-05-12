#include "Parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>


Config Parser::parseRegisters(const std::string& filename) {
    Config config;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o ficheiro: " + filename);
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // Ignorar comentários

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "registers:") {
            iss >> config.numRegisters;
        } else if (key == "algorithm:") {
            std::string algInfo;
            std::getline(iss, algInfo);
            
            algInfo.erase(0, algInfo.find_first_not_of(" \t"));
            
            size_t commaPos = algInfo.find(',');
            if (commaPos != std::string::npos) {
                config.algorithmType = algInfo.substr(0, commaPos);
                config.algorithmParam = std::stoi(algInfo.substr(commaPos + 1));
            } else {
                config.algorithmType = algInfo;
            }
        }
    }
    return config;
}


Graph<Web> Parser::parseRangesAndBuildGraph(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    std::vector<Web> allWebs;
    int webCounter = 0;

    if (!file.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o ficheiro: " + filename);
    }

 
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string varName = line.substr(0, colonPos);
        varName.erase(remove_if(varName.begin(), varName.end(), isspace), varName.end());

        std::string rangesStr = line.substr(colonPos + 1);
        std::istringstream iss(rangesStr);
        std::string token;
        
        Web tempWeb;
        tempWeb.variableName = varName;

        while (std::getline(iss, token, ',')) {
            token.erase(remove_if(token.begin(), token.end(), isspace), token.end());
            if (token.empty()) continue;

            bool isStart = (token.back() == '+');
            bool isEnd = (token.back() == '-');
            
            if (isStart || isEnd) token.pop_back();
            int lineNum = std::stoi(token);

            tempWeb.activeLines.insert(lineNum);
            if (isStart) tempWeb.startLines.insert(lineNum);
            if (isEnd) tempWeb.endLines.insert(lineNum);
        }

        tempWeb.id = webCounter++;
        allWebs.push_back(tempWeb);
    }

    Graph<Web> interferenceGraph;
    
   
    for (const auto& web : allWebs) {
        interferenceGraph.addVertex(web);
    }

    for (size_t i = 0; i < allWebs.size(); ++i) {
        for (size_t j = i + 1; j < allWebs.size(); ++j) {
            if (websInterfere(allWebs[i], allWebs[j])) {
               
                interferenceGraph.addBidirectionalEdge(allWebs[i], allWebs[j], 1.0);
            }
        }
    }

    return interferenceGraph;
}


bool Parser::websInterfere(const Web& w1, const Web& w2) {
    
    std::vector<int> commonLines;
    std::set_intersection(w1.activeLines.begin(), w1.activeLines.end(),
                          w2.activeLines.begin(), w2.activeLines.end(),
                          std::back_inserter(commonLines));

    if (commonLines.empty()) return false; 

   for (int line : commonLines) {
        bool w1Start = w1.startLines.count(line);
        bool w1End = w1.endLines.count(line);
        bool w2Start = w2.startLines.count(line);
        bool w2End = w2.endLines.count(line);

       
        if ((w1Start && w2End) || (w1End && w2Start)) {
            continue; 
        }

        
        return true; 
    }

    return false;
}