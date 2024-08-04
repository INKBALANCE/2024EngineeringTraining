#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define CHECK_INTERVAL 5 // 检测每次是否出现死锁时间

pthread_mutex_t* chopsticks;
pthread_t* philosophers;
int* philosopher_ids;
int num_philosophers;
int delay_time;
int use_non_blocking_lock = 0;
pthread_mutex_t lock;
pthread_cond_t cond;
int paused = 0;

void* philosopher(void* num) {
    int id = *(int*)num;
    int left = id - 1;
    int right = id % num_philosophers;

    while (1) {
        // 阻塞
        pthread_mutex_lock(&lock);
        while (paused) {
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);

        printf("哲学家 %d 号正在思考！\n", id);
        sleep(rand() % delay_time);

        printf("哲学家 %d 号有点饿了。\n", id);

        if (use_non_blocking_lock) {
            //非阻塞
            while (pthread_mutex_trylock(&chopsticks[left]) != 0) {
                sched_yield();
            }

            while (pthread_mutex_trylock(&chopsticks[right]) != 0) {
                pthread_mutex_unlock(&chopsticks[left]);
                sched_yield(); 
                while (pthread_mutex_trylock(&chopsticks[left]) != 0) {
                    sched_yield(); 
                }
            }
        } else {
            //加锁
            pthread_mutex_lock(&chopsticks[left]);
            pthread_mutex_lock(&chopsticks[right]);
        }

        printf("哲学家 %d 号正在食饭。\n", id);
        sleep(rand() % delay_time);

        pthread_mutex_unlock(&chopsticks[right]);
        pthread_mutex_unlock(&chopsticks[left]);

        printf("哲学家 %d 号赤饱辣！\n", id);
    }
}

int detect_deadlock() {
    for (int i = 0; i < num_philosophers; i++) {
        if (pthread_mutex_trylock(&chopsticks[i]) == 0) {
            pthread_mutex_unlock(&chopsticks[i]);
            return 0; 
        }
    }
    return 1; 
}

void clean_up() {
    for (int i = 0; i < num_philosophers; i++) {
        pthread_mutex_destroy(&chopsticks[i]);
    }
    free(chopsticks);
    free(philosophers);
    free(philosopher_ids);
}

void pause_philosophers() {
    pthread_mutex_lock(&lock);
    paused = 1;
    pthread_mutex_unlock(&lock);
}

void resume_philosophers() {
    pthread_mutex_lock(&lock);
    paused = 0;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

void setup_philosophers() {
    chopsticks = (pthread_mutex_t*)malloc(num_philosophers * sizeof(pthread_mutex_t));
    philosophers = (pthread_t*)malloc(num_philosophers * sizeof(pthread_t));
    philosopher_ids = (int*)malloc(num_philosophers * sizeof(int));

    for (int i = 0; i < num_philosophers; i++) {
        pthread_mutex_init(&chopsticks[i], NULL);
    }

    for (int i = 0; i < num_philosophers; i++) {
        philosopher_ids[i] = i + 1; 
        pthread_create(&philosophers[i], NULL, philosopher, &philosopher_ids[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "non-blocking") == 0) {
        use_non_blocking_lock = 1;
        printf("阻塞模式运行。\n");
    } else {
        printf("非阻塞模式运行。\n");
    }

    printf("输入哲学家用餐人数：");
    scanf("%d", &num_philosophers);

    printf("输入等待时间： ");
    scanf("%d", &delay_time);

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    while (1) {
        setup_philosophers();

        sleep(CHECK_INTERVAL);

        pause_philosophers();

        if (detect_deadlock()) {
            printf("哇！死锁辣！\n");
            printf("切换非阻塞运行模式？ (yes/no): ");
            char response[4];
            scanf("%3s", response);
            if (strcmp(response, "yes") == 0) {
                use_non_blocking_lock = 1;
                clean_up();
                resume_philosophers();
                continue;
            } else {
                break;
            }
        } else {
            printf("没有死锁\n");
            printf("要增加用餐人数吗？还是改变一下时间？(yes/no): ");
            char response[4];
            scanf("%3s", response);
            if (strcmp(response, "yes") == 0) {
                printf("这次你想让多少个哲学家进餐?: ");
                scanf("%d", &num_philosophers);
                printf("新的延时时长: ");
                scanf("%d", &delay_time);
                clean_up();
                resume_philosophers();
                continue;
            } else {
                break;
            }
        }

        resume_philosophers();

        for (int i = 0; i < num_philosophers; i++) {
            pthread_join(philosophers[i], NULL);
        }
        clean_up();
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return 0;
}

