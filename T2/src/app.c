#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX 2000

/*
 * PROCESSO DE APLICAÇÃO (A1, A2, A3, A4, A5) - T2
 *
 * Cada iteração do loop:
 *   1. Com 40% de chance, troca de página lógica (mem) e envia ACCESS(mem, R/W)
 *      → Isso simula a "syscall de memória" pedida pelo enunciado do T2.
 *   2. Com ~14% de chance, dispara uma syscall de I/O para D1 ou D2.
 *   3. Sempre envia UPDATE com PC e mem atual (para o relatório do Ctrl+Z).
 *
 * O Kernel gerencia a tabela de páginas: ao receber ACCESS, verifica se a
 * página está na RAM; se não estiver, causa Page Fault e bloqueia este processo.
 */

int main(int argc, char *argv[])
{
    char *nome_app = (argc > 1) ? argv[1] : "Desconhecido";
    int write_fd = (argc > 2) ? atoi(argv[2]) : -1;

    int pc = 0;
    int mem = 0; // página lógica atual (0-15)

    srand(time(NULL) ^ getpid());

    for (pc = 1; pc <= MAX; pc++)
    {

        /* ----------------------------------------------------------
         * 1) Com 40% de probabilidade, troca de página lógica e
         *    envia ACCESS(mem, op) como syscall de memória.
         * ---------------------------------------------------------- */
        if (rand() % 100 < 40 && write_fd != -1)
        {
            int nova_mem = rand() % 16;                // nova página lógica [0-15]
            char op = (nova_mem % 2 == 0) ? 'R' : 'W'; // par=leitura, ímpar=escrita

            mem = nova_mem; // atualiza estado local

            char msg_access[64];
            sprintf(msg_access, "ACCESS %s %d %c", nome_app, mem, op);
            write(write_fd, msg_access, strlen(msg_access) + 1);
            printf("App [%s] PC:%d -> ACCESS m%02d %c\n", nome_app, pc, mem, op);
        }

        /* ----------------------------------------------------------
         * 2) Com ~14% de probabilidade, faz syscall de I/O (D1 ou D2).
         * ---------------------------------------------------------- */
        if (rand() % 100 < 14 && write_fd != -1)
        {
            int d = rand() % 100;
            char dis = (d % 2 == 0) ? '1' : '2';
            char op;
            if (d % 3 == 0)
                op = 'R';
            else if (d % 3 == 1)
                op = 'W';
            else
                op = 'X';

            char msg_syscall[64];
            sprintf(msg_syscall, "SYSCALL %s D%c %c", nome_app, dis, op);
            write(write_fd, msg_syscall, strlen(msg_syscall) + 1);
            printf(">>> App [%s] PC:%d -> SYSCALL D%c %c\n", nome_app, pc, dis, op);
        }

        /* ----------------------------------------------------------
         * 3) Sempre informa PC e mem ao Kernel (para relatório Ctrl+Z).
         * ---------------------------------------------------------- */
        if (write_fd != -1)
        {
            char msg_update[64];
            sprintf(msg_update, "UPDATE %s %d %d", nome_app, pc, mem);
            write(write_fd, msg_update, strlen(msg_update) + 1);
        }

        printf("App [%s] PC:%d mem:m%02d\n", nome_app, pc, mem);

        // Simula uma unidade de tempo de processamento
        usleep(500000); // 500ms (como no enunciado)
    }

    return 0;
}