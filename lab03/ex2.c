#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void killHandler(int sinal) {
    printf("Recebi SIGKILL (%d)!\n", sinal);
}

int main(void) {
    if (signal(SIGKILL, killHandler) == SIG_ERR) {
        printf("Não foi possível interceptar o sinal SIGKILL.\n");
    }
    else{
        printf("Foi possível interceptar.\n");
    }
    while (1) {
        sleep(1);
    }
    return 0;
}