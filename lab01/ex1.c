#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {

    int mypid, pid, status;

    pid = fork();

    if (pid != 0){

        mypid = getpid();
        printf("PID Pai: %d\n", mypid);

        waitpid(-1, &status, 0);

        if (!WIFEXITED(status)) printf("O processo nao terminou corretamente\n");
    }

    else {

        mypid = getpid();
        printf("PID Filho: %d\n", mypid);
        
        exit(0);
    }

    return 0;
}