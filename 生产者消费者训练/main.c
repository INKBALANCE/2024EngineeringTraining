#include "shared.h"
#include <sys/wait.h>
#include <errno.h>

int main() {
    int num_producers, num_consumers;

    printf("Enter the number of producer processes: ");
    scanf("%d", &num_producers);
    printf("Enter the number of consumer processes: ");
    scanf("%d", &num_consumers);

    int shmid = shmget(SHM_KEY, sizeof(SharedBuffer), 0666);
    if (shmid >= 0) {
        // 删除已有的共享内存段
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl failed");
            exit(1);
        }
    } else if (errno != ENOENT) {
        perror("shmget failed");
        exit(1);
    }

    shmid = create_shared_memory();

    SharedBuffer *buffer = (SharedBuffer *)shmat(shmid, NULL, 0);
    if (buffer == (SharedBuffer *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // 初始化共享缓冲区
    buffer->in = 0;
    buffer->out = 0;
    buffer->count = 0;

    // 初始化互斥锁
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (pthread_mutex_init(&buffer->mutex, &attr) != 0) {
        perror("pthread_mutex_init failed");
        exit(1);
    }

    shmdt(buffer);

    pid_t pid;

    // 创建指定数量的生产者
    for (int i = 1; i <= num_producers; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            char producer_id[10];
            sprintf(producer_id, "%d", i);
            execlp("./producer", "producer", producer_id, NULL);
            exit(0);
        }
    }

    // 创建指定数量的消费者
    for (int i = 1; i <= num_consumers; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            char consumer_id[10];
            sprintf(consumer_id, "%d", i);
            execlp("./consumer", "consumer", consumer_id, NULL);
            exit(0);
        }
    }

    // 等待所有子进程结束
    for (int i = 0; i < (num_producers + num_consumers); i++) {
        wait(NULL);
    }

    // 清理共享内存和互斥锁
    buffer = (SharedBuffer *)shmat(shmid, NULL, 0);
    pthread_mutex_destroy(&buffer->mutex);
    shmdt(buffer);
    remove_shared_memory(shmid);

    return 0;
}

