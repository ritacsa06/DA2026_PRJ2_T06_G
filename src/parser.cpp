/**
 * @file Parser.cpp
 * @brief Implementation of the Parser class for reading configuration and live ranges.
 */

#include "Parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

/**
 * @brief Reads the registers file and extracts the allocation configuration.
 * * @details Parses lines looking for "registers:" and "algorithm:" keywords, 
 * handling optional algorithm parameters separated by commas.
 * <b>Time Complexity:</b> O(L), where L is the number of lines in the configuration file.
 */
Config Parser::parseRegisters(const std::string& filename) {
    Config config;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o ficheiro: " + filename);
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

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

/**
 * @brief Reads the live ranges file, constructs webs, and builds the interference graph (T1.2).
 * * @details Reads the file line by line. Uses a greedy algorithm to merge overlapping 
 * or contiguous live ranges belonging to the same variable into a single unified Web.
 * After assigning definitive IDs, it builds the interference graph by checking 
 * execution point overlaps between all pairs of webs using `websInterfereGlobal`.
 * <b>Time Complexity:</b> O(L + W^2 * P), where L is the number of lines in the file, 
 * W is the final number of merged webs, and P is the average number of active program points per web.
 */
Graph<Web> Parser::parseRangesAndBuildGraph(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    std::vector<Web> parsedRanges;

    if (!file.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o ficheiro: " + filename);
    }

    // 1. Read file line by line
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
        parsedRanges.push_back(tempWeb);
    }

    // 2. Greedy algorithm to merge overlapping Live Ranges of the same variable
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < parsedRanges.size(); ++i) {
            for (size_t j = i + 1; j < parsedRanges.size(); ++j) {
                if (parsedRanges[i].variableName == parsedRanges[j].variableName) {
                    std::vector<int> intersection;
                    std::set_intersection(parsedRanges[i].activeLines.begin(), parsedRanges[i].activeLines.end(),
                                          parsedRanges[j].activeLines.begin(), parsedRanges[j].activeLines.end(),
                                          std::back_inserter(intersection));
                    
                    if (!intersection.empty()) {
                        // Merge active, start, and end lines
                        parsedRanges[i].activeLines.insert(parsedRanges[j].activeLines.begin(), parsedRanges[j].activeLines.end());
                        parsedRanges[i].startLines.insert(parsedRanges[j].startLines.begin(), parsedRanges[j].startLines.end());
                        parsedRanges[i].endLines.insert(parsedRanges[j].endLines.begin(), parsedRanges[j].endLines.end());
                        
                        // Cancel markers at the merge point (if one range ends where another begins)
                        std::vector<int> toCancel;
                        for (int x : parsedRanges[i].startLines) {
                            if (parsedRanges[i].endLines.count(x)) {
                                toCancel.push_back(x);
                            }
                        }
                        for (int x : toCancel) {
                            parsedRanges[i].startLines.erase(x);
                            parsedRanges[i].endLines.erase(x);
                        }
                        
                        parsedRanges.erase(parsedRanges.begin() + j);
                        changed = true;
                        break;
                    }
                }
            }
            if (changed) break;
        }
    }

    // 3. Assign definitive IDs to the merged Webs
    int webCounter = 0;
    for (auto& web : parsedRanges) {
        web.id = webCounter++;
    }

    // 4. Build the Interference Graph
    Graph<Web> interferenceGraph;
    for (const auto& web : parsedRanges) {
        interferenceGraph.addVertex(web);
    }

    for (size_t i = 0; i < parsedRanges.size(); ++i) {
        for (size_t j = i + 1; j < parsedRanges.size(); ++j) {
            // Use the globally refactored interference function
            if (websInterfereGlobal(parsedRanges[i], parsedRanges[j])) {
                interferenceGraph.addBidirectionalEdge(parsedRanges[i], parsedRanges[j], 1.0);
            }
        }
    }

    return interferenceGraph;
}