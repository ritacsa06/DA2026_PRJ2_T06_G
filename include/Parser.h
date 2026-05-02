#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Web.h"

struct Config {
    int numRegisters = 0;
    std::string algorithmType = "basic";
    int algorithmParam = 0; 
};

class Parser {
public:
    // Lê o ficheiro registers.txt e extrai as configurações
    static Config parseRegisters(const std::string& filename);

    // Lê o ficheiro ranges.txt, cria as Webs e constrói o Grafo de Interferência
    static Graph<Web> parseRangesAndBuildGraph(const std::string& filename);

private:
    
    static void mergeIntoWebs(std::vector<Web>& webs, const std::string& varName, 
                              const std::vector<std::string>& tokens);
    static bool websInterfere(const Web& w1, const Web& w2);
};

#endif