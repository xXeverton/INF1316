#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main() {
    // Cria ou abre o arquivo com permissão de leitura e escrita
    int fd = open("mem_compartilhada.dat", O_RDWR | O_CREAT, 0666);
    if (fd == -1) { perror("open"); exit(1); }
    
    // Define o tamanho do arquivo para 4096 bytes (1 página)
    ftruncate(fd, 4096); 
    
    // Mapeia o arquivo na memória como COMPARTILHADO (MAP_SHARED)
    char *p = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { perror("mmap"); exit(1); }
    
    // Escreve uma mensagem no início da página de memória
    strcpy(p, "Ola do Programa 1!");
    printf("Prog1: Mensagem '%s' escrita na memoria.\n", p);
    
    // Desfaz o mapeamento e fecha
    munmap(p, 4096);
    close(fd);
    
    return 0;
}