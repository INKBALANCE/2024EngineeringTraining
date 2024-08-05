#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8888
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 10

typedef struct {
    int socket;
    struct sockaddr_in address;
    char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_t thread_pool[THREAD_POOL_SIZE];

typedef struct task {
    void (*function)(void *);
    void *arg;
    struct task *next;
} task_t;

typedef struct {
    task_t *front;
    task_t *rear;
    int count;
} task_queue_t;

task_queue_t task_queue = { NULL, NULL, 0 };

void enqueue_task(task_t *new_task) {
    pthread_mutex_lock(&queue_mutex);
    if (task_queue.rear == NULL) {
        task_queue.front = new_task;
        task_queue.rear = new_task;
    } else {
        task_queue.rear->next = new_task;
        task_queue.rear = new_task;
    }
    task_queue.count++;
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

task_t *dequeue_task() {
    pthread_mutex_lock(&queue_mutex);
    while (task_queue.count == 0) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }
    task_t *task = task_queue.front;
    task_queue.front = task_queue.front->next;
    if (task_queue.front == NULL) {
        task_queue.rear = NULL;
    }
    task_queue.count--;
    pthread_mutex_unlock(&queue_mutex);
    return task;
}

void broadcast_message(char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket != sender_socket) {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                perror("Failed to send message");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void handle_client(void *arg) {
    char buffer[BUFFER_SIZE];
    int leave_flag = 0;
    client_t *cli = (client_t *)arg;

    // 接收客户端编号信息
    int receive = recv(cli->socket, cli->name, 32, 0);
    cli->name[receive] = '\0';

    // 验证客户端编号唯一性
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && strcmp(clients[i]->name, cli->name) == 0) {
            char message[] = "该编号已被使用\n";
            send(cli->socket, message, sizeof(message), 0);
            close(cli->socket);
            free(cli);
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
    }

    // 将客户端添加到客户列表中
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = cli;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "%s 已连接成功\n", cli->name);
    printf("%s", message);
    send(cli->socket, message, sizeof(message), 0);

    while (1) {
        receive = recv(cli->socket, buffer, BUFFER_SIZE, 0);
        if (receive > 0) {
            buffer[receive] = '\0';
            printf("%s\n", buffer);
            broadcast_message(buffer, cli->socket);
        } else if (receive == 0 || strcmp(buffer, "/exit\n") == 0) {
            leave_flag = 1;
        } else {
            perror("Failed to receive message");
            leave_flag = 1;
        }
        if (leave_flag) {
            break;
        }
    }

    close(cli->socket);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == cli) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    snprintf(message, sizeof(message), "%s 已退出连接\n", cli->name);
    printf("%s", message);
    broadcast_message(message, cli->socket);  // 向其他客户端广播客户端下线消息
    free(cli);
}

void *thread_function(void *arg) {
    while (1) {
        task_t *task = dequeue_task();
        if (task) {
            task->function(task->arg);
            free(task);
        }
    }
    return NULL;
}

void *server_message_handler(void *arg) {
    char message[BUFFER_SIZE];

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        char full_message[BUFFER_SIZE + 8];
        snprintf(full_message, sizeof(full_message), "Server: %s", message);
        printf("%s", full_message); // 在服务器终端上显示服务器发送的消息
        broadcast_message(full_message, -1); // -1 表示来自服务器的消息
    }

    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_message_handler, NULL);

    printf("服务器在 %d 端口上启动\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Client accept failed");
            exit(EXIT_FAILURE);
        }

        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->socket = client_socket;
        cli->address = client_addr;

        task_t *new_task = (task_t *)malloc(sizeof(task_t));
        new_task->function = handle_client;
        new_task->arg = cli;
        new_task->next = NULL;
        enqueue_task(new_task);
    }

    close(server_socket);
    return 0;
}

