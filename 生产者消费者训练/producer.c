#include "shared.h"
#include<errno.h>
void produce(SharedBuffer *buffer, int producer_id) {
    char filename[20];
    sprintf(filename, "producer%d.txt", producer_id);

    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Failed to open producer file");
        exit(1);
    }

    while (1) {
        int item = rand() % 100;

        // 加锁
        pthread_mutex_lock(&buffer->mutex);

        // 放入缓冲区
        buffer->buffer[buffer->in] = item;
        buffer->in = (buffer->in + 1) % BUFFER_SIZE;
        buffer->count++;
        printf("%d号生产者生产数据%d\n", producer_id, item);

        // 将数据写入文件
        fprintf(file, "%d号生产者生产数据%d\n", producer_id, item);
        fflush(file);

        // 解锁
        pthread_mutex_unlock(&buffer->mutex);

        sleep(2); // 间隔2秒钟
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s producer_id\n", argv[0]);
        exit(1);
    }
    int producer_id = atoi(argv[1]);

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

    produce(buffer, producer_id);

    shmdt(buffer);

    return 0;
}

