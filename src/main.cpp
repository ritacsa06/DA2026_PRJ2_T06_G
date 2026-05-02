#include <iostream>
#include <string>
#include <vector>
#include "Parser.h"

void processAllocation(const std::string& rangesFile, const std::string& registersFile, const std::string& outputFile) {
    std::cout << "\n--- Iniciando Processamento ---" << std::endl;
    
    // T1.2: Ler as configurações (Registers)
    std::cout << "[1] A ler configuracoes de: " << registersFile << std::endl;
    Config config = Parser::parseRegisters(registersFile);
    
    std::cout << "    -> Registos disponiveis: " << config.numRegisters << std::endl;
    std::cout << "    -> Algoritmo escolhido: " << config.algorithmType 
              << " (Parametro: " << config.algorithmParam << ")" << std::endl;

    // T1.2: Ler os Live Ranges e Construir o Grafo
    std::cout << "[2] A ler Live Ranges e a construir o Grafo de Interferencias de: " << rangesFile << std::endl;
    Graph<Web> interferenceGraph = Parser::parseRangesAndBuildGraph(rangesFile);
    
    std::cout << "    -> Sucesso! Grafo construido com " << interferenceGraph.getNumVertex() << " Webs (variaveis)." << std::endl;

    // TODO: T2 - Construir e correr a classe Allocator (usando config e interferenceGraph)
    // TODO: T1.1/T3 - Escrever o resultado final no outputFile
    
    std::cout << "\n(Fase T1.2 Concluida! O Grafo esta pronto para colorir na proxima fase.)\n" << std::endl;
}


int runBatchMode(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Erro: Numero incorreto de argumentos no modo batch." << std::endl;
        std::cerr << "Uso correto: " << argv[0] << " -b <ranges.txt> <registers.txt> <allocation.txt>" << std::endl;
        return 1;
    }

    std::string rangesFile = argv[2];
    std::string registersFile = argv[3];
    std::string outputFile = argv[4];

    try {
        processAllocation(rangesFile, registersFile, outputFile);
    } catch (const std::exception& e) {
        std::cerr << "Erro fatal durante a execucao em batch: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void displayMenu() {
    std::cout << "\n=========================================\n";
    std::cout << "  Compilador - Alocador de Registos (DA)   \n";
    std::cout << "=========================================\n";
    std::cout << "1. Correr Alocacao de Registos\n";
    std::cout << "0. Sair\n";
    std::cout << "=========================================\n";
    std::cout << "Escolha uma opcao: ";
}

int runInteractiveMode() {
    int choice = -1;
    std::string rangesFile, registersFile, outputFile;

    while (choice != 0) {
        displayMenu();
        if (!(std::cin >> choice)) {
            std::cerr << "Erro: Entrada invalida. Introduza um numero." << std::endl;
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        switch (choice) {
            case 1:
                std::cout << "\nIntroduza o caminho para o ficheiro de Live Ranges (ex: data/ranges/ranges1.txt): ";
                std::cin >> rangesFile;
                std::cout << "Introduza o caminho para o ficheiro de Registos (ex: data/registers/registers1.txt): ";
                std::cin >> registersFile;
                std::cout << "Introduza o caminho para o ficheiro de Output (ex: data/output/test.txt): ";
                std::cin >> outputFile;
                
                try {
                    processAllocation(rangesFile, registersFile, outputFile);
                } catch (const std::exception& e) {
                    std::cerr << "\nErro durante a alocacao: " << e.what() << std::endl;
                }
                break;
            case 0:
                std::cout << "A encerrar o programa. Adeus!" << std::endl;
                break;
            default:
                std::cerr << "Opcao invalida. Tente novamente." << std::endl;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string flag = argv[1];
        if (flag == "-b") {
            return runBatchMode(argc, argv);
        } else {
            std::cerr << "Erro: Flag desconhecida '" << flag << "'." << std::endl;
            std::cerr << "Para usar o modo batch utilize a flag '-b'." << std::endl;
            return 1;
        }
    } 
    else {
        return runInteractiveMode();
    }
}