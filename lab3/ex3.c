#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h> 

void childhandler(int signo);
int delay; 

int main(int argc, char *argv[]) {
    pid_t pid;
    
    sscanf(argv[1], "%d", &delay);
    
    signal(SIGCHLD, childhandler);

    if ((pid = fork()) < 0) {
        fprintf(stderr, "Erro ao criar filho\n");
        exit(-1);
    }

    if (pid == 0) { 
        char *args[] = {argv[2], NULL};
        execve(argv[2], args, NULL);
        perror("execve"); 
        exit(1);
    } else { 
        sleep(delay);
        printf("Program %s exceeded limit of %d seconds!\n", argv[2], delay);
        kill(pid, SIGKILL);
        
        sleep(1); 
    }

    return 0;
}

void childhandler(int signo) {
    int status;
    pid_t pid = wait(&status);
    
    if (WIFEXITED(status)) {
        printf("Child %d terminated normally within %d seconds com estado %d.\n", pid, delay, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Child %d was killed by signal %d.\n", pid, WTERMSIG(status));
    }
    
    exit(0);
}