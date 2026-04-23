#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fdEntrada, fdSaida;
    char buffer[100];
    int n;

    fdEntrada = open("entrada.txt", O_RDONLY);
    if (fdEntrada < 0) { perror("Erro ao abrir entrada"); exit(1); }

    fdSaida = open("saida.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fdSaida < 0) { perror("Erro ao abrir saida"); exit(1); }

    dup2(fdEntrada, 0);
    close(fdEntrada);    

    dup2(fdSaida, 1);
    close(fdSaida);    

    fprintf(stderr, "Redirecionando entrada.txt -> saida.txt...\n");

    while ((n = read(0, buffer, sizeof(buffer))) > 0) {
        if (write(1, buffer, n) < 0) {
            perror("Erro ao escrever");
            exit(1);
        }
    }

    fprintf(stderr, "Concluído.\n");
    return 0;
}