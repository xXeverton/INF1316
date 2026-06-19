#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "common.h"

/*
 * KERNELSIM - Simulador de Kernel Preemptivo
 *
 * Gerencia 5 processos de aplicação usando escalonamento Round-Robin com preemption.
 * Responde a três tipos de interrupções:
 * - IRQ0: TimeSlice (força troca de contexto)
 * - IRQ1: Hardware D1 completa uma operação (desbloqueia processos)
 * - IRQ2: Hardware D2 completa uma operação (desbloqueia processos)
 *
 * Estados dos processos:
 * 0 = PRONTO (aguardando CPU)
 * 1 = EXECUTANDO (em posse da CPU)
 * 2 = BLOQUEADO (aguardando E/S)
 * 3 = TERMINADO (finalizou seus MAX ciclos)
 */

/*
 * Lê uma mensagem completa do pipe terminada em \0
 * As mensagens vêm do InterController (interrupções) ou dos processos (UPDATE/SYSCALL)
 */
int ler_mensagem_pipe(int fd, char *buffer)
{
    int i = 0;
    char c;
    // Lê 1 único caractere por vez
    while (read(fd, &c, 1) > 0)
    {
        buffer[i++] = c;
        if (c == '\0')
        {
            return i; // Mensagem completa encontrada e separada!
        }
    }
    return 0; // Pipe vazio ou fechado
}

// ----- INICIALIZAÇÃO DO KERNELSIM E CRIAÇÃO DOS PROCESSOS -----

void run_kernel(int read_fd, int write_fd)
{
    signal(SIGTSTP, handle_sigtstp);

    char buffer[50];
    printf("KernelSim iniciado e a aguardar interrupções...\n");

    char fd_str[10];
    sprintf(fd_str, "%d", write_fd);

    // Cria os 5 processos de aplicação
    for (int i = 0; i < 5; i++)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            printf("Erro ao criar processo filho\n");
            exit(1);
        }

        if (pid == 0)
        {
            // Filho executa o programa da aplicação
            execl("./bin/app", "./bin/app", nomes[i], fd_str, NULL);
            perror("Falha no execl\n");
            exit(1);
        }
        else
        {
            // Pai armazena o PID e pausa imediatamente
            processos[i] = pid;
            usleep(10000);
            kill(pid, SIGSTOP);
            printf("Kernel: Processo %s (PID %d) criado e pausado\n", nomes[i], pid);
        }
    }

    // Filas para processos bloqueados aguardando I/O
    int fila_d1[5], tamanho_d1 = 0;
    int fila_d2[5], tamanho_d2 = 0;
    // Fila para guardar os ID's dos processos que estão
    // à espera de páginas
    int fila_swap[5], tamanho_swap = 0;

    // Escalonador Round-Robin: começa pelo primeiro processo
    int processo_atual = 0;
    kill(processos[processo_atual], SIGCONT);
    estado_processos[processo_atual] = 1;
    printf("Kernel: Iniciando a execução com %s (PID %d)\n", nomes[processo_atual], processos[processo_atual]);

    // Contador para saber se quadro preencheu
    int quadros_ocupados = 0;        // vai de 0 a 32
    int ponteiro_fifo = 0;          // Aponta para o quadro (0 a 31) que será a próxima vítima


    // ----- LOOP PRINCIPAL: PROCESSA INTERRUPÇÕES E GERENCIA PROCESSOS -----

    while (1)
    {
        int status;

        // Verifica se algum processo finalizou sua execução
        for (int i = 0; i < 5; i++)
        {
            if (estado_processos[i] != 3)
            {
                if (waitpid(processos[i], &status, WNOHANG) > 0)
                {
                    estado_processos[i] = 3;
                    printf("\n>>> Kernel: Processo %s finalizou sua execucao! <<<\n\n", nomes[i]);
                }

                // Verifica se todos os processos terminaram
                int processos_terminados = 0;
                for (int i = 0; i < 5; i++)
                {
                    if (estado_processos[i] == 3)
                        processos_terminados++;
                }

                // Se todos terminaram, encerra o simulador
                if (processos_terminados == 5)
                {
                    printf("\n=======================================================\n");
                    printf(">>> TODOS OS PROCESSOS APLICAÇÃO TERMINARAM <<<\n");
                    printf(">>> INICIANDO DESLIGAMENTO DO KERNEL...     <<<\n");
                    printf("=======================================================\n");
                    kill(0, SIGKILL);
                }
            }
        }

        // Aguarda uma mensagem do pipe
        if (ler_mensagem_pipe(read_fd, buffer) > 0)
        {

            // IRQ0: TimeSlice terminou - força troca de contexto (preemption)
            if (strcmp(buffer, "IRQ0") == 0)
            {
                // Pausa o processo atual se estiver executando
                if (estado_processos[processo_atual] == 1)
                {
                    kill(processos[processo_atual], SIGSTOP);
                    estado_processos[processo_atual] = 0;
                }

                // Encontra o próximo processo PRONTO (em round-robin)
                for (int j = 0; j < 5; j++)
                {
                    processo_atual = (processo_atual + 1) % 5;
                    if (estado_processos[processo_atual] == 0)
                    {
                        estado_processos[processo_atual] = 1;
                        kill(processos[processo_atual], SIGCONT);
                        printf(
                            "--- Troca de Contexto por IRQ0: Agora rodando %s ---\n",
                            nomes[processo_atual]);
                        break;
                    }
                }
            }

            // // UPDATE: Recebe informação de estado de um processo (PC e memória)
            // else if (strncmp(buffer, "UPDATE", 6) == 0)
            // {
            //     int id = buffer[8] - '1';
            //     int lido_pc, lido_mem;
            //     sscanf(buffer, "UPDATE %*s %d %d", &lido_pc, &lido_mem);

            //     pc_processos[id] = lido_pc;
            //     mem_processos[id] = lido_mem;
            // }
            // UPDATE: Recebe informação de estado de um processo (PC e memória)
            else if (strncmp(buffer, "UPDATE", 6) == 0)
            {
                char pid_str[3];
                int num_pc, num_mem;
                sscanf(buffer, "UPDATE %s %d %d", pid_str, &num_pc, &num_mem);
                int id_processo = pid_str[1] - '1';

                pc_processos[id_processo] = num_pc;
                mem_processos[id_processo] = num_mem;

                // ====================================================================
                // T2: MMU SIMULADA E TRATAMENTO DE PAGE FAULT
                // Só testamos se o processo estiver ativamente a correr (estado 1)
                // ====================================================================
                if (estado_processos[id_processo] == 1)
                {
                    // Verifica na Tabela de Páginas se a página Lógica (num_mem) está na RAM
                    if (tabelas_paginas[id_processo][num_mem].valid == 0)
                    {
                        // PAGE FAULT! A página não está na RAM.
                        printf(
                            ">>> [PAGE FAULT] Processo %s (pag logica %d) nao esta na RAM!\n",
                            nomes[id_processo], num_mem);

                        // Incrementa o contador de faltas de página para o relatório final
                        page_faults[id_processo]++;

                        // (A LÓGICA DE BLOQUEAR O PROCESSO E PEDIR AO SWAP ENTRARÁ AQUI NA FASE 2)
                        // Por enquanto, apenas imprimimos a mensagem e deixamos o processo continuar,
                        // para garantir que não quebramos o escalonador do T1.
                        estado_processos[id_processo] = 2; // Bloqueado

                        // Pausa a execução do processo
                        kill(processos[id_processo], SIGSTOP);

                        fila_swap[tamanho_swap++] = id_processo;

                        // Encontra o próximo processo PRONTO (em round-robin)
                        for (int j = 0; j < 5; j++)
                        {
                            processo_atual = (processo_atual + 1) % 5;
                            if (estado_processos[processo_atual] == 0)
                            {
                                estado_processos[processo_atual] = 1;
                                kill(processos[processo_atual], SIGCONT);
                                printf(
                                    "--- Troca de Contexto por IRQ0: Agora rodando %s ---\n",
                                    nomes[processo_atual]);
                                break;
                            }
                        }
                    }
                    else
                    {
                        // PAGE HIT! A página já está num quadro da RAM. Tudo OK.
                        // Podemos opcionalmente imprimir um log ou apenas não fazer nada.
                    }
                }
                // ====================================================================
            }

            // SYSCALL: Um processo solicita uma operação de E/S e é bloqueado
            else if (strncmp(buffer, "SYSCALL", 7) == 0)
            {
                // Formato: "SYSCALL A1 D1 R"
                printf("Kernel recebeu: %s \n", buffer);

                int id_bloqueado = buffer[9] - '1';
                disp_bloqueado[id_bloqueado] = buffer[12];
                oper_bloqueado[id_bloqueado] = buffer[14];

                // Conta os acessos a cada dispositivo (para o relatório)
                if (buffer[12] == '1')
                    io_counts_d1[id_bloqueado]++;
                else if (buffer[12] == '2')
                    io_counts_d2[id_bloqueado]++;

                // Pausa o processo imediatamente
                kill(processos[id_bloqueado], SIGSTOP);
                estado_processos[id_bloqueado] = 2;

                // Coloca na fila de espera do dispositivo apropriado
                if (buffer[12] == '1')
                {
                    fila_d1[tamanho_d1++] = id_bloqueado;
                    printf("Kernel: Processo %s foi bloqueado e colocado na Fila D1.\n", nomes[id_bloqueado]);
                }
                else if (buffer[12] == '2')
                {
                    fila_d2[tamanho_d2++] = id_bloqueado;
                    printf("Kernel: Processo %s foi bloqueado e colocado na Fila D2.\n", nomes[id_bloqueado]);
                }

                // Se o processo que pediu I/O era o que estava executando,
                // precisa escolher outro para rodar na CPU
                if (processo_atual == id_bloqueado)
                {
                    for (int j = 0; j < 5; j++)
                    {
                        processo_atual = (processo_atual + 1) % 5;
                        if (estado_processos[processo_atual] == 0)
                        {
                            estado_processos[processo_atual] = 1;
                            kill(processos[processo_atual], SIGCONT);
                            printf(
                                "--- Troca de Contexto FORCADA por Syscall: Agora rodando %s ---\n",
                                nomes[processo_atual]);
                            break;
                        }
                    }
                }
            }

            // IRQ1: Dispositivo D1 completou uma operação de E/S
            else if (strcmp(buffer, "IRQ1") == 0)
            {
                if (tamanho_d1 > 0)
                {
                    // Desbloqueia o primeiro processo que está aguardando D1
                    int id_acordado = fila_d1[0];
                    estado_processos[id_acordado] = 0;
                    printf(
                        ">>> Kernel: Hardware D1 terminou! Processo %s foi DESBLOQUEADO e agora esta PRONTO.\n",
                        nomes[id_acordado]);

                    // Remove o processo da fila (FIFO)
                    for (int k = 0; k < tamanho_d1 - 1; k++)
                    {
                        fila_d1[k] = fila_d1[k + 1];
                    }
                    tamanho_d1--;
                }
            }

            // IRQ2: Dispositivo D2 completou uma operação de E/S
            else if (strcmp(buffer, "IRQ2") == 0)
            {
                if (tamanho_d2 > 0)
                {
                    // Desbloqueia o primeiro processo que está aguardando D2
                    int id_acordado = fila_d2[0];
                    estado_processos[id_acordado] = 0;
                    printf(
                        ">>> Kernel: Hardware D2 terminou! Processo %s foi DESBLOQUEADO e agora esta PRONTO.\n",
                        nomes[id_acordado]);

                    // Remove o processo da fila (FIFO)
                    for (int k = 0; k < tamanho_d2 - 1; k++)
                    {
                        fila_d2[k] = fila_d2[k + 1];
                    }
                    tamanho_d2--;
                }
            }

            // IRQ3: Disco termina de buscar a página
            else if (strcmp(buffer, "IRQ3") == 0)
            {
                if (tamanho_swap > 0)
                {
                    // Desbloqueia o primeiro processo que está aguardando
                    int id_desbloqueado = fila_swap[0];
                    estado_processos[id_desbloqueado] = 0;
                    printf
                    (
                        ">>> Kernel: Disco de Swap terminou (IRQ3)! Processo %s foi DESBLOQUEADO e agora esta PRONTO.\n",
                        nomes[id_desbloqueado]
                    );

                    // Remove o processo da fila de espera deslocando os demais
                    for (int k = 0; k < tamanho_swap - 1; k++)
                    {
                        fila_swap[k] = fila_swap[k + 1];
                    }
                    tamanho_swap--;
                    
                    // // A página agora está na RAM!
                    // tabelas_paginas[id_desbloqueado][mem_processos[id_desbloqueado]].valid = 0;

                    // O processo e a página que causaram o fault
                    int pagina_solicitada = mem_processos[id_desbloqueado];

                    // Cenário A: tem espaço na RAM
                    if (quadros_ocupados < 32)
                    {
                        int quadro_livre = quadros_ocupados;

                        // Atualiza a tabela de páginas do processo
                        tabelas_paginas[id_desbloqueado][pagina_solicitada].valid = 1;
                        tabelas_paginas[id_desbloqueado][pagina_solicitada].frame = quadro_livre;

                        // Atualiza as estruturas globais de RAM
                        memoria_ram[quadro_livre].id_processo = id_desbloqueado;
                        memoria_ram[quadro_livre].pagina_logica = pagina_solicitada;

                        ram_free[quadro_livre] = 0;

                        quadros_ocupados++;

                        printf(
                            ">>> Kernel: Pagina %d do Processo %s carregada no Quadro %d.\\n",
                            pagina_solicitada,
                            nomes[id_desbloqueado],
                            quadro_livre
                        );
                    }
                }
            }
        }
    }
}

// ----- RELATÓRIO DE ESTADO (EXIBIDO AO APERTAR CTRL+Z) -----

void handle_sigtstp(int sig)
{
    printf("\n\n=======================================================\n");
    printf("     SIMULAÇÃO PAUSADA (Ctrl+Z) - STATUS DO SISTEMA    \n");
    printf("=======================================================\n");

    for (int i = 0; i < 5; i++)
    {
        printf("Processo [%s]:\n", nomes[i]);
        printf("  - PC Atual      : %d\n", pc_processos[i]);
        printf("  - Memoria       : m%02d\n", mem_processos[i]);
        printf("  - Acessos a D1  : %d vezes\n", io_counts_d1[i]);
        printf("  - Acessos a D2  : %d vezes\n", io_counts_d2[i]);

        printf("  - Estado        : ");
        if (estado_processos[i] == 0)
            printf("PRONTO\n");
        else if (estado_processos[i] == 1)
            printf("EXECUTANDO (CPU)\n");
        else if (estado_processos[i] == 2)
            printf("BLOQUEADO (Aguardando D%c, operacao %c)\n", disp_bloqueado[i], oper_bloqueado[i]);
        else if (estado_processos[i] == 3)
            printf("TERMINADO\n");

        printf("-------------------------------------------------------\n");
    }

    printf("\nPressione [ENTER] para retomar a simulação...\n");
    getchar();

    printf("Retomando simulação...\n");
    printf("=======================================================\n\n");
}