#include <iostream>
#include <string>
#include <vector>
#include "Parser.h"
#include "Allocator.h"
#include "Writer.h"

void processAllocation(const std::string& rangesFile,
                       const std::string& registersFile,
                       const std::string& outputFile) {

    std::cout << "\n--- Iniciando Processamento ---" << std::endl;

    // T1.2 – Read configuration (registers + algorithm)
    std::cout << "[1] A ler configuracoes de: " << registersFile << std::endl;
    Config config = Parser::parseRegisters(registersFile);

    std::cout << "    -> Registos disponiveis : " << config.numRegisters << std::endl;
    std::cout << "    -> Algoritmo escolhido  : " << config.algorithmType
              << " (Parametro: " << config.algorithmParam << ")" << std::endl;

    // T1.2 – Read live ranges and build interference graph
    std::cout << "[2] A ler Live Ranges e a construir o Grafo de Interferencias de: "
              << rangesFile << std::endl;
    Graph<Web> interferenceGraph = Parser::parseRangesAndBuildGraph(rangesFile);

    std::cout << "    -> Grafo construido com "
              << interferenceGraph.getNumVertex() << " Webs (variaveis)." << std::endl;

    // T2.x – Run the allocator (dispatch by algorithm type)
    std::cout << "[3] A executar o algoritmo de alocacao de registos..." << std::endl;

    Allocator allocator(interferenceGraph, config.numRegisters);
    AllocationResult result;

    if (config.algorithmType == "basic") {
        // T2.1: basic greedy coloring, no controlled spilling
        result = allocator.allocate();

    } else if (config.algorithmType == "spilling") {
        // T2.2: greedy coloring with up to K controlled web spills
        int maxSpills = (config.algorithmParam > 0) ? config.algorithmParam : 1;
        std::cout << "    -> Modo spilling: maximo de " << maxSpills
                  << " web(s) permitida(s) para memoria." << std::endl;
        result = allocator.allocateWithSpilling(maxSpills);

    } else if (config.algorithmType == "splitting") {
        // T2.3: greedy coloring with up to K web splits
        int maxSplits = (config.algorithmParam > 0) ? config.algorithmParam : 1;
        std::cout << "    -> Modo splitting: maximo de " << maxSplits
                  << " split(s) permitido(s)." << std::endl;
        result = allocator.allocateWithSplitting(maxSplits);

    } else if (config.algorithmType == "free") {
        // T2.4: Algoritmo livre customizado (Cost-Benefit Spilling)
        std::cout << "    -> Modo livre [T2.4]: A executar alocacao inteligente por Custo-Beneficio." << std::endl;
        result = allocator.allocateFree();

    } else {
        // Fallback to basic for unrecognised algorithm types
        std::cerr << "    [AVISO] Algoritmo '" << config.algorithmType
                  << "' nao reconhecido. A usar 'basic'." << std::endl;
        result = allocator.allocate();
    }

    if (result.success) {
        std::cout << "    -> Alocacao bem-sucedida! Registos utilizados: "
                  << result.registersUsed << std::endl;
    } else {
        std::cout << "    -> Alocacao com spilling! "
                  << result.websSpilled << " web(s) enviada(s) para memoria." << std::endl;
    }

    // T1.1 – Write output file
    std::cout << "[4] A escrever resultado em: " << outputFile << std::endl;
    Writer::write(result, outputFile);

    std::cout << "\n(Processamento concluido!)\n" << std::endl;
}

int runBatchMode(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Erro: Numero incorreto de argumentos no modo batch." << std::endl;
        std::cerr << "Uso correto: " << argv[0]
                  << " -b <ranges.txt> <registers.txt> <allocation.txt>" << std::endl;
        return 1;
    }

    std::string rangesFile    = argv[2];
    std::string registersFile = argv[3];
    std::string outputFile    = argv[4];

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
                std::cout << "\nIntroduza o caminho para o ficheiro de Live Ranges"
                             " (ex: data/ranges/ranges1.txt): ";
                std::cin >> rangesFile;
                std::cout << "Introduza o caminho para o ficheiro de Registos"
                             " (ex: data/registers/registers1.txt): ";
                std::cin >> registersFile;
                std::cout << "Introduza o caminho para o ficheiro de Output"
                             " (ex: data/output/test.txt): ";
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
    } else {
        return runInteractiveMode();
    }
}