#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(void)
{
    int mypid, pid, status;
    int numero = 1;

    printf("numero = %d (antes do processo pai)\n", numero);
    pid = fork();

    if (pid != 0)
    {
        mypid = getpid();
        printf("Parent PID: %d\n", mypid);
        waitpid(-1, &status, 0);
        printf("numero = %d (apos processo filho)\n", numero);
        
        if (!WIFEXITED(status)) printf("O processo nao terminou corretamente\n");

    }
    else
    {

        numero = 5;
        mypid = getpid();
        printf("Child PID: %d\n", mypid);
        
        printf("numero = %d (durante processo filho)\n", numero);
        exit(0);
    }

    return 0;

}