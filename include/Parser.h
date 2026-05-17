#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Web.h"

/**
 * @brief Configuration settings extracted from the registers input file.
 */
struct Config {
    int numRegisters = 0;                 ///< The maximum number of physical registers available (K).
    std::string algorithmType = "basic";  ///< The chosen allocation algorithm (e.g., "basic", "spilling", "splitting", "free").
    int algorithmParam = 0;               ///< Optional numeric parameter for the algorithm (e.g., max spills or max splits).
};

/**
 * @brief Static utility class responsible for parsing input files and constructing the foundational data structures.
 * @details Handles the extraction of configuration settings and the processing of 
 * variable live ranges to build the interference graph used by the allocator.
 */
class Parser {
public:
    /**
     * @brief Reads the registers file and extracts the allocation configuration.
     * * @details Parses lines looking for "registers:" and "algorithm:" keywords, 
     * handling optional algorithm parameters separated by commas.
     * <b>Time Complexity:</b> O(L), where L is the number of lines in the configuration file.
     * @param filename Path to the registers text file.
     * @return A Config struct containing the parsed settings.
     * @throws std::runtime_error if the file cannot be opened.
     */
    static Config parseRegisters(const std::string& filename);

    /**
     * @brief Reads the live ranges file, constructs webs, and builds the interference graph (T1.2).
     * * @details Reads the file line by line. Uses a greedy algorithm to merge overlapping 
     * or contiguous live ranges belonging to the same variable into a single unified Web.
     * After assigning definitive IDs, it builds the interference graph by checking 
     * execution point overlaps between all pairs of webs using `websInterfereGlobal`.
     * <b>Time Complexity:</b> O(L + W^2 * P), where L is the number of lines in the file, 
     * W is the final number of merged webs, and P is the average number of active program points per web.
     * @param filename Path to the live ranges text file.
     * @return A Graph<Web> representing the interference graph.
     * @throws std::runtime_error if the file cannot be opened.
     */
    static Graph<Web> parseRangesAndBuildGraph(const std::string& filename);

};

#endif // PARSER_H