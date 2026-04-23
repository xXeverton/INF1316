#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>   // <-- adicionado

int main() {
    int fd[2];
    pid_t pid;
    char mensagem[] = "Mensagem enviada pelo filho";
    char buffer[100];

    if (pipe(fd) < 0) { perror("Erro ao criar pipe"); exit(1); }

    pid = fork();
    if (pid < 0) { perror("Erro no fork"); exit(1); }

    if (pid == 0) {                          // FILHO
        close(fd[0]);
        if (write(fd[1], mensagem, strlen(mensagem) + 1) < 0)
            perror("Erro ao escrever");
        printf("Filho escreveu: %s\n", mensagem);
        close(fd[1]);

    } else {                                 // PAI
        close(fd[1]);
        ssize_t n = read(fd[0], buffer, sizeof(buffer));
        if (n > 0) printf("Pai leu: %s\n", buffer);
        close(fd[0]);
        wait(NULL);                          // <-- adicionado
    }

    return 0;
}