#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {
    struct stat sb;
    int fd;
    char *p;
    struct timeval start, end;
    long pagesize;
    int num_pages;

    if (argc < 2) { 
        fprintf(stderr, "usage: %s <file>\n", argv[0]); 
        exit(1); 
    }

    // Abrimos em O_RDWR para permitir testar a escrita de forma justa
    fd = open(argv[1], O_RDWR);
    if (fd == -1) { perror("open"); exit(1); }
    if (fstat(fd, &sb) == -1) { perror("fstat"); exit(1); }

    pagesize = sysconf(_SC_PAGESIZE); // Obtém o tamanho da página do SO
    
    // Calcula as páginas arredondando para cima
    num_pages = (sb.st_size + pagesize - 1) / pagesize; 
    
    printf("\n--- ESTATÍSTICAS DE MEMÓRIA ---\n");
    printf("Tamanho do arquivo:  %ld bytes\n", sb.st_size);
    printf("Tamanho da página:   %ld bytes\n", pagesize);
    printf("Páginas alocadas:    %d\n\n", num_pages);

    printf("--- TESTE DE DESEMPENHO ---\n");

    // TESTE 1: mmap()
    gettimeofday(&start, NULL); // Inicia cronômetro mmap
    
    p = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { perror("mmap"); exit(1); }

    // Lendo e reescrevendo o arquivo inteiro via mapeamento
    for (off_t i = 0; i < sb.st_size; i++) {
        char c = p[i]; // Lê o byte
        p[i] = c;      // Escreve o mesmo byte de volta
    }
    
    munmap(p, sb.st_size);
    
    gettimeofday(&end, NULL); // Para cronômetro mmap
    double tempo_mmap = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Tempo de execução com mmap():       %f segundos\n", tempo_mmap);


    // TESTE 2: E/S tradicional (read / write)
    lseek(fd, 0, SEEK_SET); // Volta o ponteiro do disco para o início
    
    gettimeofday(&start, NULL); // Inicia cronômetro E/S
    
    char buffer;
    for (off_t i = 0; i < sb.st_size; i++) {
        read(fd, &buffer, 1);         // Lê um byte
        lseek(fd, -1, SEEK_CUR);      // Volta um espaço
        write(fd, &buffer, 1);        // Escreve de volta
    }
    
    gettimeofday(&end, NULL); // Para cronômetro E/S
    double tempo_io = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Tempo de execução com E/S normal:   %f segundos\n\n", tempo_io);

    close(fd);
    return 0;
}