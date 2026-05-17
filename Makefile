# Variáveis de Compilação
CXX = g++
CXXFLAGS = -Wall -Wextra -Iinclude -std=c++17 -O2
LDFLAGS = 

# Pastas
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
BIN_DIR = .


TARGET = myProg

# Descoberta automática de ficheiros fonte e objetos
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# Regra principal
all: $(TARGET)

# Linkagem do executável
$(TARGET): $(OBJS)
	@echo "A compilar o executável final: $@"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compilação dos ficheiros objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Gerar documentação Doxygen 
doxygen:
	@echo "A gerar documentação com Doxygen..."
	doxygen Doxyfile

# Limpeza do projeto
clean:
	@echo "A limpar ficheiros temporários e executável..."
	rm -rf $(OBJ_DIR) $(TARGET)
	rm -rf docs/html latex

# Comandos "Phony" para evitar conflitos com ficheiros do mesmo nome
.PHONY: all clean doxygen