# Nome do compilador
CXX = g++

# Flags de compilação (-std=c++17 para usar o C++17, -g para debugging, -Wall para warnings)
CXXFLAGS = -std=c++17 -Wall -g

# Pasta para armazenar arquivos .o
OBJDIR = .build

# Arquivos de origem
SRC = Utils.cpp ConfigManager.cpp FileManager.cpp Peer.cpp TCPServer.cpp UDPServer.cpp main.cpp

# Arquivos de cabeçalho
HEADERS = Constants.h Utils.h ConfigManager.h FileManager.h Peer.h TCPServer.h UDPServer.h

# Nome do executável
TARGET = p2p

# Converte os arquivos .cpp para .o adicionando-os na pasta .build
OBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC))

# Regra padrão para construir o executável
all: $(OBJDIR) $(TARGET)

# Regra para criar o diretório .build, caso ele não exista
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Constrói o executável a partir dos objetos
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

# Regra para compilar os arquivos .cpp em arquivos .o na pasta .build
$(OBJDIR)/%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza de arquivos gerados (.o e executável)
clean:
	rm -f $(OBJDIR)/*.o $(TARGET)
