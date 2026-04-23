#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>    // <-- adicionado

int main() {
    int fd[2];
    pid_t pid1, pid2;

    if (pipe(fd) < 0) { perror("Erro ao criar pipe"); exit(1); }

    // FILHO 1 → ps
    pid1 = fork();
    if (pid1 < 0) { perror("Erro no fork 1"); exit(1); }
    if (pid1 == 0) {
        close(fd[0]);
        dup2(fd[1], 1);
        close(fd[1]);
        execlp("ps", "ps", "aux", NULL);   // <-- ps aux mais completo
        perror("Erro no execlp ps");
        exit(1);
    }

    // FILHO 2 → wc
    pid2 = fork();
    if (pid2 < 0) { perror("Erro no fork 2"); exit(1); }
    if (pid2 == 0) {
        close(fd[1]);
        dup2(fd[0], 0);
        close(fd[0]);
        execlp("wc", "wc", "-l", NULL);    // <-- wc -l conta linhas
        perror("Erro no execlp wc");
        exit(1);
    }

    // PAI
    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, NULL, 0);    // <-- específico
    waitpid(pid2, NULL, 0);    // <-- específico

    return 0;
}