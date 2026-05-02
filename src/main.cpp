#include <iostream>
#include <string>
#include <vector>

// Funções de simulação das próximas fases (T1.2 e T2)
void processAllocation(const std::string& rangesFile, const std::string& registersFile, const std::string& outputFile) {
    // TODO: T1.2 - Ligar ao Parser para ler rangesFile e registersFile
    // TODO: T2 - Construir o Grafo e correr o Allocator
    // TODO: T1.1 - Escrever o resultado no outputFile
    
    std::cout << "A processar alocação..." << std::endl;
    std::cout << "- Ranges: " << rangesFile << std::endl;
    std::cout << "- Registos: " << registersFile << std::endl;
    std::cout << "- Output: " << outputFile << std::endl;
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
                std::cout << "\nIntroduza o caminho para o ficheiro de Live Ranges (ex: data/ranges.txt): ";
                std::cin >> rangesFile;
                std::cout << "Introduza o caminho para o ficheiro de Registos (ex: data/registers.txt): ";
                std::cin >> registersFile;
                std::cout << "Introduza o caminho para o ficheiro de Output (ex: alloc.txt): ";
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