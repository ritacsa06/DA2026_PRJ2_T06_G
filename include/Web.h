#ifndef WEB_H
#define WEB_H

#include <string>
#include <set>
#include <vector>
#include <algorithm>

/**
 * @brief Constant representing an unassigned register or a web sent to memory (spilled).
 */
static constexpr int NO_REGISTER = -1;

/**
 * @brief Represents a live range or a merged set of live ranges for a variable.
 * @details A Web tracks the exact program points where a specific variable is active.
 * It is used as the foundational vertex information inside the interference graph.
 */
struct Web {
    int id = -1;                  ///< Unique identifier for the web.
    std::string variableName;     ///< The original variable name from the source code.

    std::set<int> activeLines;    ///< Set of all program points where the variable is live.
    std::set<int> startLines;     ///< Set of program points where the variable is defined/starts.
    std::set<int> endLines;       ///< Set of program points where the variable is last used/ends.

    int assignedRegister = NO_REGISTER; ///< The physical register assigned by the allocator.

    /**
     * @brief Equality operator to compare two webs based on their unique ID.
     * <b>Time Complexity:</b> O(1).
     * @param other The other web to compare against.
     * @return True if both webs have the same ID.
     */
    bool operator==(const Web& other) const {
        return id == other.id;
    }
};

/**
 * @brief Global inline function to determine if two webs interfere.
 * @details Two webs interfere if they share at least one active program point. 
 * However, if they intersect at a point where one web exactly ends (last use) 
 * and the other exactly begins (definition), they do not interfere at that specific point.
 * <b>Time Complexity:</b> O(L_1 + L_2), where L_1 and L_2 are the number of active lines in w1 and w2 respectively, driven by `std::set_intersection`.
 * @param w1 The first web.
 * @param w2 The second web.
 * @return True if there is a conflict (interference) between the two webs, false otherwise.
 */
inline bool websInterfereGlobal(const Web& w1, const Web& w2) {
    std::vector<int> commonLines;
    
    // Find the intersection of active lines
    std::set_intersection(w1.activeLines.begin(), w1.activeLines.end(),
                          w2.activeLines.begin(), w2.activeLines.end(),
                          std::back_inserter(commonLines));
                          
    if (commonLines.empty()) return false; 
    
    // Check for boundary conditions (def-use overlap)
    for (int line : commonLines) {
        bool w1Start = w1.startLines.count(line), w1End = w1.endLines.count(line);
        bool w2Start = w2.startLines.count(line), w2End = w2.endLines.count(line);
        
        // If one ends and the other starts at the same line, no interference here
        if ((w1Start && w2End) || (w1End && w2Start)) continue;
        
        return true; 
    }
    
    return false;
}

#endif // WEB_H