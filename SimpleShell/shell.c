#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE 80 // 最长操作量
#define MAX_ARGS 20 // 内存量

void parseInput(char* input, char** args) {
    for (int i = 0; i < MAX_ARGS; i++) {
        args[i] = strsep(&input, " ");
        if (args[i] == NULL) break;
    }
}

int main(void) {
    char* args[MAX_ARGS + 1]; 
    char input[MAX_LINE]; //装入缓冲区

    while (1) {
        printf("osh> ");
        fflush(stdout);

//读取缓冲区
        if (fgets(input, MAX_LINE, stdin) == NULL) {
            perror("fgets failed");
            exit(1);
        }

从输入读取命令
        input[strlen(input) - 1] = '\0';

        parseInput(input, args);

//退出指令
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        pid_t pid = fork();

        if (pid < 0) { 
            perror("fork failed");
            exit(1);
        } else if (pid == 0) { // 子进程
            printf("Child process (PID: %d, Parent PID: %d)\n", getpid(), getppid());
            if (execvp(args[0], args) < 0) {
                perror("execvp failed");
                exit(1);
            }
        } else { // 父进程
            printf("Parent process (PID: %d, Child PID: %d)\n", getpid(), pid);
            wait(NULL); 
            printf("Child process finished.\n");
        }
    }

    return 0;
}

