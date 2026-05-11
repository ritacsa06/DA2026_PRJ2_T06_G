#ifndef WEB_H
#define WEB_H

#include <string>
#include <set>
#include <vector>

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

#endif // WEB_H