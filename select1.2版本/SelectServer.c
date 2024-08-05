// SelectServer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct {
    int socket;
    struct sockaddr_in address;
    char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char *message, int sender_socket);

void *server_input_handler(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        fgets(buffer, BUFFER_SIZE - 16, stdin); // 减去"服务器: "和结尾的空间
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
        if (strlen(buffer) > 0) {
            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), "服务器: ");
            strncat(message, buffer, sizeof(message) - strlen(message) - 1);
            strncat(message, "\n", sizeof(message) - strlen(message) - 1);
            printf("%s", message);
            broadcast_message(message, -1); // -1 indicates message from server
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

    int server_fd, new_socket, max_sd, activity, valread, sd;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = NULL;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening on %s:%d\n", ip, port);

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_input_handler, NULL);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i] ? clients[i]->socket : 0;
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int addrlen = sizeof(address);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("New connection: socket fd is %d, ip is %s, port %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            client_t *new_client = (client_t *)malloc(sizeof(client_t));
            new_client->socket = new_socket;
            new_client->address = address;

            int name_len = read(new_socket, new_client->name, 32);
            new_client->name[name_len] = '\0';

            int name_exists = 0;
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] && strcmp(clients[i]->name, new_client->name) == 0) {
                    name_exists = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);

            if (name_exists) {
                char message[] = "该编号已被使用\n";
                send(new_socket, message, sizeof(message), 0);
                close(new_socket);
                free(new_client);
                continue;
            }

            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i]) {
                    clients[i] = new_client;
                    printf("Adding to list of sockets as %d\n", i + 1);
                    snprintf(buffer, BUFFER_SIZE, "%s 已连接成功\n", new_client->name);
                    send(new_socket, buffer, strlen(buffer), 0);
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i] ? clients[i]->socket : 0;
            if (FD_ISSET(sd, &readfds)) {
                int addrlen = sizeof(address);
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Host disconnected: ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    snprintf(buffer, BUFFER_SIZE, "%s 已退出连接\n", clients[i]->name);
                    broadcast_message(buffer, sd);
                    close(sd);
                    free(clients[i]);
                    clients[i] = NULL;
                } else {
                    buffer[valread] = '\0';
                    printf("Message from client %s: %s\n", clients[i]->name, buffer);
                    char msg_with_id[BUFFER_SIZE];
                    snprintf(msg_with_id, sizeof(msg_with_id), "%s: ", clients[i]->name);
                    strncat(msg_with_id, buffer, sizeof(msg_with_id) - strlen(msg_with_id) - 1);
                    broadcast_message(msg_with_id, sd);
                }
            }
        }

        int connected_clients = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i]) connected_clients++;
        }
        printf("Connected clients: %d\n", connected_clients);
    }

    return 0;
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

