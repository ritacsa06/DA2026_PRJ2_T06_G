/**
 * @file main.cpp
 * @brief Entry point for the Register Allocator Compiler module.
 * @details Handles command-line arguments, provides an interactive CLI, and orchestrates the parsing, allocation, and writing processes.
 */

#include <iostream>
#include <string>
#include <vector>
#include "Parser.h"
#include "Allocator.h"
#include "Writer.h"

/**
 * @brief Core pipeline orchestrator for register allocation.
 *
 * @details Executes the complete flow of the program:
 * 1. Parses configuration (available registers and algorithm type).
 * 2. Parses live ranges, merges them into Webs, and builds the interference graph.
 * 3. Dispatches the appropriate register allocation algorithm (basic, spilling, splitting, or free).
 * 4. Writes the final allocation to the output file strictly following the project spec.
 *
 * <b>Time Complexity:</b> Dominated by the allocation algorithm chosen:
 * - Basic: O(V^2 + E)
 * - Spilling/Splitting: O(S * (V^2 + E)) or O(S * V^2 * L)
 * - Free: O(V * (V^2 + E))
 *
 * @param rangesFile    Path to the live ranges input file.
 * @param registersFile Path to the registers configuration input file.
 * @param outputFile    Path where the final allocation result will be saved.
 */
void processAllocation(const std::string& rangesFile,
                       const std::string& registersFile,
                       const std::string& outputFile) {

    std::cout << "\n--- Iniciando Processamento ---" << std::endl;

    std::cout << "[1] A ler configuracoes de: " << registersFile << std::endl;
    Config config = Parser::parseRegisters(registersFile);

    std::cout << "    -> Registos disponiveis : " << config.numRegisters << std::endl;
    std::cout << "    -> Algoritmo escolhido  : " << config.algorithmType
              << " (Parametro: " << config.algorithmParam << ")" << std::endl;

    std::cout << "[2] A ler Live Ranges e a construir o Grafo de Interferencias de: "
              << rangesFile << std::endl;
    Graph<Web> interferenceGraph = Parser::parseRangesAndBuildGraph(rangesFile);

    std::cout << "    -> Grafo construido com "
              << interferenceGraph.getNumVertex() << " Webs (variaveis)." << std::endl;

    std::cout << "[3] A executar o algoritmo de alocacao de registos..." << std::endl;

    Allocator allocator(interferenceGraph, config.numRegisters);
    AllocationResult result;

    if (config.algorithmType == "basic") {
        result = allocator.allocate();

    } else if (config.algorithmType == "spilling") {
        int maxSpills = (config.algorithmParam > 0) ? config.algorithmParam : 1;
        std::cout << "    -> Modo spilling: maximo de " << maxSpills
                  << " web(s) permitida(s) para memoria." << std::endl;
        result = allocator.allocateWithSpilling(maxSpills);

    } else if (config.algorithmType == "splitting") {
        int maxSplits = (config.algorithmParam > 0) ? config.algorithmParam : 1;
        std::cout << "    -> Modo splitting: maximo de " << maxSplits
                  << " split(s) permitido(s)." << std::endl;
        result = allocator.allocateWithSplitting(maxSplits);

    } else if (config.algorithmType == "free") {
        std::cout << "    -> Modo livre [T2.4]: A executar alocacao inteligente por Custo-Beneficio." << std::endl;
        result = allocator.allocateFree();

    } else {
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

    std::cout << "[4] A escrever resultado em: " << outputFile << std::endl;
    Writer::write(result, outputFile);

    std::cout << "\n(Processamento concluido!)\n" << std::endl;
}

/**
 * @brief Executes the program in batch mode using command-line arguments.
 *
 * @details Validates argument count and extracts file paths to run the allocation pipeline without user interaction.
 * <b>Time Complexity:</b> O(1) execution overhead + Time Complexity of `processAllocation`.
 *
 * @param argc Argument count.
 * @param argv Argument vector (expects: ./prog -b <ranges> <registers> <output>).
 * @return 0 on success, 1 on argument error or fatal execution exception.
 */
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

/**
 * @brief Displays the interactive CLI menu to the standard output.
 * <b>Time Complexity:</b> O(1).
 */
void displayMenu() {
    std::cout << "\n=========================================\n";
    std::cout << "  Compilador - Alocador de Registos (DA)   \n";
    std::cout << "=========================================\n";
    std::cout << "1. Correr Alocacao de Registos\n";
    std::cout << "0. Sair\n";
    std::cout << "=========================================\n";
    std::cout << "Escolha uma opcao: ";
}

/**
 * @brief Executes the program in an interactive CLI loop.
 *
 * @details Prompts the user for input/output paths and processes them until the user chooses to exit.
 * <b>Time Complexity:</b> O(1) overhead per iteration + Time Complexity of `processAllocation`.
 *
 * @return 0 upon clean exit.
 */
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

/**
 * @brief Main entry point of the program.
 *
 * @details Routes execution to either batch mode (if "-b" flag is provided) or interactive mode.
 * <b>Time Complexity:</b> O(1) overhead.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Program exit status code (0 for success).
 */
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