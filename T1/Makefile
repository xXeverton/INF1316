# Define qual compilador usar
CC = gcc

# Define as flags do compilador (avisos e modo de depuração)
CFLAGS = -Wall -g

# Define as pastas do projeto
SRC_DIR = src
BIN_DIR = bin

# Define o nome do executável final
EXEC = $(BIN_DIR)/simulador

# Arquivos fonte
SRC = $(SRC_DIR)/main.c $(SRC_DIR)/kernel.c $(SRC_DIR)/controller.c $(SRC_DIR)/app.c

# Regra principal: o que acontece quando você digita "make"
all: $(EXEC)

# Como construir o executável
$(EXEC): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC)

# Regra para limpar os arquivos compilados (digite "make clean")
clean:
	rm -f $(EXEC)