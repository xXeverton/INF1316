INF1316 - Trabalho 2: Simulador de Micro-Kernel e SFSS (UDP)
Objetivo
Este projeto estende um simulador de núcleo de Sistema Operacional (KernelSim), adicionando suporte a um Sistema de Arquivos Remoto (SFSS). O Kernel gerencia processos de aplicação que realizam leitura, escrita e manipulação de diretórios remotos, comunicando-se com o servidor através de um protocolo customizado sobre UDP (SFP - Simple File Protocol).

Estrutura de Arquivos
kernel.c: O micro-kernel. Gerencia o escalonamento Round-Robin, comunicação com as aplicações via memória compartilhada e o enfileiramento das respostas UDP.

sfss.c: O servidor remoto stateless. Escuta a rede, executa as operações nos arquivos físicos locais e devolve os dados e offsets via UDP.

app.c: Os processos de aplicação (A1 a A5). Rodam em loop consumindo CPU virtual e sorteando System Calls de I/O repassadas ao kernel via pipe.

controller.c: O simulador de interrupções. Gera os sinais de hardware para o kernel: IRQ0 (clock) e IRQ1/IRQ2 (conclusão de I/O na rede).

main.c: Ponto de entrada do sistema. Inicializa os pipes de comunicação e cria os processos base (fork).

common.h: Define a estrutura das mensagens SFP (Datagramas UDP) e as tabelas globais da simulação (PCB).

Makefile: Script para automatizar a compilação do projeto.

Como Compilar e Executar
1. Criar a árvore de diretórios do servidor
Como o servidor é stateless, os diretórios físicos base dos processos precisam existir antes da execução. Rode os comandos abaixo no terminal:

mkdir -p SFSS-root-dir/A0/dir_teste
mkdir -p SFSS-root-dir/A1/dir_teste
mkdir -p SFSS-root-dir/A2/dir_teste
mkdir -p SFSS-root-dir/A3/dir_teste
mkdir -p SFSS-root-dir/A4/dir_teste
mkdir -p SFSS-root-dir/A5/dir_teste

2. Compilar o projeto
Na mesma pasta dos arquivos fonte, execute o Makefile:

make

3. Iniciar o Servidor (SFSS)
Em um terminal, inicie o servidor remoto para que ele fique em escuta na porta 8080:

./sfss

4. Iniciar o Micro-Kernel (Simulação)
Abra um novo terminal, navegue até a pasta do projeto e inicie a simulação:

./t2

Controles de Diagnóstico
Pausar e Inspecionar: No terminal do ./t2, pressione Ctrl+Z para pausar a simulação e imprimir o relatório completo de estado dos processos (PCBs) e o tamanho das filas de rede. Pressione ENTER para retomar.

Encerrar: Pressione Ctrl+C em ambos os terminais para finalizar a execução e liberar as portas de rede.