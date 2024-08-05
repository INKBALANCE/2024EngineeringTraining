// TCPClient.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int sock;
    struct sockaddr_in server_addr;
    pthread_t receive_thread;
    char buffer[BUFFER_SIZE];
    char client_id[ID_SIZE];

    printf("请输入客户端编号：");
    fgets(client_id, ID_SIZE, stdin);
    client_id[strcspn(client_id, "\n")] = 0; // 去除换行符

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // 发送客户端编号
    send(sock, client_id, strlen(client_id), 0);

    char response[BUFFER_SIZE];
    int receive = recv(sock, response, BUFFER_SIZE, 0);
    response[receive] = '\0';
    if (strcmp(response, "该编号已被使用\n") == 0) {
        printf("%s", response);
        close(sock);
        return 0;
    } else {
        printf("%s", response);
    }

    pthread_create(&receive_thread, NULL, receive_messages, &sock);

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        if (strcmp(buffer, "quit\n") == 0) {
            send(sock, buffer, strlen(buffer), 0);
            break;
        }
        // 在发送的消息前附加客户端编号
        char full_message[BUFFER_SIZE];
        snprintf(full_message, sizeof(full_message), "%s", buffer);
        if (send(sock, full_message, strlen(full_message), 0) < 0) {
            perror("Failed to send message");
        }
    }

    close(sock);
    return 0;
}

