#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int main (int argc, char *argv[]) {
    struct stat sb;
    int fd;
    off_t len;
    char *p;

    // Verifica se o usuário passou o nome do arquivo
    if (argc < 2) { 
        fprintf (stderr, "usage: %s <file>\n", argv[0]); 
        exit(1); 
    }

    // Abre o arquivo apenas para leitura
    fd = open (argv[1], O_RDONLY);
    if (fd == -1) { 
        perror ("open"); 
        exit(1); 
    }

    // Obtém as informações do arquivo (como o tamanho total em bytes)
    if (fstat(fd, &sb) == -1) { 
        perror ("fstat");
        exit(1);
    }

    // Verifica se é um arquivo regular
    if (!S_ISREG (sb.st_mode)) { 
        fprintf (stderr, "%s is not a file\n", argv[1]); 
        exit(1);
    }

    // Mapeia o arquivo na memória
    p = mmap (0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { 
        perror ("mmap"); 
        exit(1); 
    }

    // Como o arquivo já está mapeado na memória, podemos fechar o descritor
    if (close (fd) == -1) { 
        perror ("close"); 
        exit(1); 
    }

    // Lemos o conteúdo do arquivo como se fosse um simples array de caracteres na memória!
    for (len = 0; len < sb.st_size; len++) {
        putchar (p[len]);
    }

    // Desfaz o mapeamento da memória
    if (munmap (p, sb.st_size) == -1) {
        perror ("munmap"); 
        exit(1); 
    }

    return 0;
}