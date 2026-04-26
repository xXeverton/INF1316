# Define qual compilador usar e as flags
CC = gcc
CFLAGS = -Wall -g

# Regra principal: o que acontece quando você digita "make"
# Queremos que ele construa TANTO o simulador QUANTO a app
all: bin/simulador bin/app

# Regra para construir o Simulador (Kernel + Controller)
# Por enquanto, tudo isso está dentro do main.c
bin/simulador: src/main.c
	$(CC) $(CFLAGS) src/main.c -o bin/simulador

# Regra para construir a Aplicação Trabalhadora
bin/app: src/app.c
	$(CC) $(CFLAGS) src/app.c -o bin/app

# Regra para limpar tudo
clean:
	rm -f bin/simulador bin/app