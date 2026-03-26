#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void fpe_handler(int signum) {
    printf("\nErro: divisão por zero detectada!\n");
    exit(1);
}

int main() {
    int a, b;

    signal(SIGFPE, fpe_handler);

    printf("Digite dois números: ");
    scanf("%d %d", &a, &b);

    printf("Soma: %d\n", a + b);
    printf("Subtração: %d\n", a - b);
    printf("Multiplicação: %d\n", a * b);

    printf("Divisão: %d\n", a / b);

    return 0;
}