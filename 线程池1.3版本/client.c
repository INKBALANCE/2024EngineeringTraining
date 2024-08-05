#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define ID_SIZE 32

void *receive_messages(void *socket) {
    int sockfd = *(int *)socket;
    char buffer[BUFFER_SIZE];
    while (1) {
        int receive = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (receive > 0) {
            buffer[receive] = '\0';
            printf("%s", buffer);
        } else if (receive == 0) {
            break;
        } else {
            perror("Failed to receive message");
        }
    }
    return NULL;
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    pthread_t receive_thread;
    char message[BUFFER_SIZE];
    char client_id[ID_SIZE];

    printf("请输入客户端编号：");
    fgets(client_id, ID_SIZE, stdin);
    client_id[strcspn(client_id, "\n")] = 0; // 去除换行符

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to the server failed");
        exit(EXIT_FAILURE);
    }

    // 发送客户端编号
    send(sockfd, client_id, strlen(client_id), 0);

    char buffer[BUFFER_SIZE];
    int receive = recv(sockfd, buffer, BUFFER_SIZE, 0);
    buffer[receive] = '\0';
    if (strcmp(buffer, "该编号已被使用\n") == 0) {
        printf("%s", buffer);
        close(sockfd);
        return 0;
    } else {
        printf("%s", buffer);
    }

    pthread_create(&receive_thread, NULL, receive_messages, &sockfd);

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        if (strcmp(message, "/exit\n") == 0) {
            send(sockfd, message, strlen(message), 0);
            break;
        }
        // 在发送的消息前附加客户端编号
        char full_message[BUFFER_SIZE];
        snprintf(full_message, sizeof(full_message), "%s: ", client_id);
        strncat(full_message, message, sizeof(full_message) - strlen(full_message) - 1);
        if (send(sockfd, full_message, strlen(full_message), 0) < 0) {
            perror("Failed to send message");
        }
    }

    close(sockfd);
    return 0;
}

