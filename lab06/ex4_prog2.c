#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main() {
    // Abre o arquivo criado pelo prog1
    int fd = open("mem_compartilhada.dat", O_RDWR);
    if (fd == -1) { perror("open"); exit(1); }
    
    // Mapeia o arquivo na memória
    char *p = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { perror("mmap"); exit(1); }
    
    // Lê a mensagem deixada pelo Programa 1 no início do arquivo
    printf("Prog2: Li a seguinte mensagem -> '%s'\n", p);
    
    // Escreve uma resposta no mesmo arquivo (deslocando 50 bytes para não sobrescrever)
    strcpy(p + 50, "Ola do Programa 2, mensagem recebida com sucesso!");
    printf("Prog2: Resposta escrita na memoria.\n");
    
    munmap(p, 4096);
    close(fd);
    
    return 0;
}