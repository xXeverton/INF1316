#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(void)
{
int mypid, pid, status;


pid = fork();

if (pid != 0)
{
    mypid = getpid();
    printf("Parent PID: %d\n", mypid);
    waitpid(-1, &status, 0);

    if (!WIFEXITED(status)) printf("O processo nao terminou corretamente\n");

}
else
{
    mypid = getpid();
    printf("Child PID: %d\n", mypid);

    // execl("./processos", "processos", NULL);
    execlp("echo", "echo", "alo mundo", NULL);

    printf("Erro: o exec falhou!\n");

    exit(1);
}

return 0;

}