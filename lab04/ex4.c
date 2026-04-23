#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>    // <-- adicionado

int main() {
    int fd[2];
    pid_t leitor1, leitor2, escritor;
    char buffer[100];
    int n;

    if (pipe(fd) < 0) { perror("Erro ao criar pipe"); exit(1); }

    // ESCRITOR
    escritor = fork();
    if (escritor < 0) { perror("Erro no fork escritor"); exit(1); }
    if (escritor == 0) {
        close(fd[0]);
        for (int i = 1; i <= 10; i++) {
            char msg[50];
            snprintf(msg, sizeof(msg), "Mensagem %d\n", i);  // <-- snprintf
            if (write(fd[1], msg, strlen(msg)) < 0) {
                perror("Erro ao escrever"); exit(1);
            }
            printf("[Escritor] enviou: %s", msg);
            fflush(stdout);                                   // <-- fflush
            sleep(1);
        }
        close(fd[1]);
        exit(0);
    }

    // LEITOR 1
    leitor1 = fork();
    if (leitor1 < 0) { perror("Erro no fork leitor1"); exit(1); }
    if (leitor1 == 0) {
        close(fd[1]);
        while ((n = read(fd[0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[n] = '\0';
            printf("[Leitor 1] recebeu: %s", buffer);
            fflush(stdout);                                   // <-- fflush
            sleep(2);
        }
        close(fd[0]);
        exit(0);
    }

    // LEITOR 2
    leitor2 = fork();
    if (leitor2 < 0) { perror("Erro no fork leitor2"); exit(1); }
    if (leitor2 == 0) {
        close(fd[1]);
        while ((n = read(fd[0], buffer, sizeof(buffer)-1)) > 0) {
            buffer[n] = '\0';
            printf("[Leitor 2] recebeu: %s", buffer);
            fflush(stdout);                                   // <-- fflush
            sleep(2);
        }
        close(fd[0]);
        exit(0);
    }

    // PAI
    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    return 0;
}