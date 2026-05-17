#ifndef WEB_H
#define WEB_H

#include <string>
#include <set>
#include <vector>
#include <algorithm>

static constexpr int NO_REGISTER = -1;

struct Web {
    int id = -1;                  
    std::string variableName;    

    std::set<int> activeLines;    
    std::set<int> startLines;     
    std::set<int> endLines;       

    int assignedRegister = NO_REGISTER;

    bool operator==(const Web& other) const {
        return id == other.id;
    }
};

inline bool websInterfereGlobal(const Web& w1, const Web& w2) {
    std::vector<int> commonLines;
    std::set_intersection(w1.activeLines.begin(), w1.activeLines.end(),
                          w2.activeLines.begin(), w2.activeLines.end(),
                          std::back_inserter(commonLines));
    if (commonLines.empty()) return false; 
    for (int line : commonLines) {
        bool w1Start = w1.startLines.count(line), w1End = w1.endLines.count(line);
        bool w2Start = w2.startLines.count(line), w2End = w2.endLines.count(line);
        if ((w1Start && w2End) || (w1End && w2Start)) continue;
        return true; 
    }
    return false;
}
#endif // WEB_H