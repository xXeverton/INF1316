CC = gcc
CFLAGS = -Wall -g

# A regra 'all' é a padrão. Quando você digita 'make', ele compila tudo isso.
all: sfss app t2

# Compila o Servidor Stateless
sfss: sfss.c common.h
	$(CC) $(CFLAGS) -o sfss sfss.c

# Compila o Processo de Aplicação (que será chamado via execl)
app: app.c common.h
	$(CC) $(CFLAGS) -o app app.c

# Compila o Kernel e o Controller juntos na main
t2: main.c kernel.c controller.c common.h
	$(CC) $(CFLAGS) -o t2 main.c kernel.c controller.c

# Regra para limpar os executáveis quando você digitar 'make clean'
clean:
	rm -f sfss app t2