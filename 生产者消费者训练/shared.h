#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>

#define BUFFER_SIZE 10 // 缓冲区大小
#define SHM_KEY 0x1234 // 共享内存键值

typedef struct {
    int buffer[BUFFER_SIZE];
    int in;
    int out;
    int count;
    pthread_mutex_t mutex;
} SharedBuffer;

int create_shared_memory();
void remove_shared_memory(int shmid);

#endif

