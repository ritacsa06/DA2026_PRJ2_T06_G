#ifndef WEB_H
#define WEB_H

#include <string>
#include <set>
#include <vector>

struct Web {
    int id;                     
    std::string variableName;   
    

    std::set<int> activeLines;  
    
    
    std::set<int> startLines;   
    std::set<int> endLines;     


    bool operator==(const Web& other) const {
        return id == other.id;
    }
};

#endif