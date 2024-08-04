#include "shared.h"
#include<errno.h>

void consume(SharedBuffer *buffer, int consumer_id) {
    char filename[20];
    sprintf(filename, "consumer%d.txt", consumer_id);

    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Failed to open consumer file");
        exit(1);
    }

    while (1) {
        // 加锁
        pthread_mutex_lock(&buffer->mutex);

        // 从缓冲区取出
        if (buffer->count > 0) {
            int item = buffer->buffer[buffer->out];
            buffer->out = (buffer->out + 1) % BUFFER_SIZE;
            buffer->count--;
            printf("%d号消费者读入数据%d\n", consumer_id, item);

            // 将数据写入文件
            fprintf(file, "%d号消费者读入数据%d\n", consumer_id, item);
            fflush(file);
        }

        // 解锁
        pthread_mutex_unlock(&buffer->mutex);

        sleep(2); // 间隔2秒钟
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s consumer_id\n", argv[0]);
        exit(1);
    }
    int consumer_id = atoi(argv[1]);

    int shmid = shmget(SHM_KEY, sizeof(SharedBuffer), 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }

    SharedBuffer *buffer = (SharedBuffer *)shmat(shmid, NULL, 0);
    if (buffer == (SharedBuffer *)-1) {
        perror("shmat failed");
        exit(1);
    }

    consume(buffer, consumer_id);

    shmdt(buffer);

    return 0;
}

