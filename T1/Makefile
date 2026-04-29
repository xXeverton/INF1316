CC = gcc
CFLAGS = -Wall -g -I./src

all: bin/simulador bin/app

# Compila o simulador juntando os três arquivos C
bin/simulador: src/main.c src/kernel.c src/controller.c
	$(CC) $(CFLAGS) src/main.c src/kernel.c src/controller.c -o bin/simulador

# Compila a aplicação trabalhadora
bin/app: src/app.c
	$(CC) $(CFLAGS) src/app.c -o bin/app

clean:
	rm -f bin/simulador bin/app