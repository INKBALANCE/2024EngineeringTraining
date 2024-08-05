// EpollServer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define MAX_EVENTS 100

typedef struct {
    int socket;
    struct sockaddr_in address;
    char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];

void broadcast_message(char *message, int sender_socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket != sender_socket) {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                perror("Failed to send message");
            }
        }
    }
}

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

    int server_fd, new_socket, epoll_fd;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    struct epoll_event ev, events[MAX_EVENTS];
    socklen_t addrlen;

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

    if ((epoll_fd = epoll_create1(0)) < 0) {
        perror("epoll_create failed");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening on %s:%d\n", ip, port);

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_input_handler, NULL);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; n++) {
            if (events[n].data.fd == server_fd) {
                addrlen = sizeof(address);
                if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                    perror("accept failed");
                    continue;
                }

                printf("New connection: socket fd is %d, ip is %s, port %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                client_t *new_client = (client_t *)malloc(sizeof(client_t));
                new_client->socket = new_socket;
                new_client->address = address;

                int name_len = read(new_socket, new_client->name, 32);
                new_client->name[name_len] = '\0';

                int name_exists = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] && strcmp(clients[i]->name, new_client->name) == 0) {
                        name_exists = 1;
                        break;
                    }
                }

                if (name_exists) {
                    char message[] = "该编号已被使用\n";
                    send(new_socket, message, sizeof(message), 0);
                    close(new_socket);
                    free(new_client);
                    continue;
                }

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i]) {
                        clients[i] = new_client;
                        printf("Adding to list of sockets as %d\n", i + 1);
                        snprintf(buffer, BUFFER_SIZE, "%s 已连接成功\n", new_client->name);
                        send(new_socket, buffer, strlen(buffer), 0);
                        break;
                    }
                }

                ev.events = EPOLLIN;
                ev.data.fd = new_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) < 0) {
                    perror("epoll_ctl failed");
                    exit(EXIT_FAILURE);
                }
            } else {
                int sd = events[n].data.fd;
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    addrlen = sizeof(address);
                    getpeername(sd, (struct sockaddr *)&address, &addrlen);
                    printf("Host disconnected: ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i] && clients[i]->socket == sd) {
                            snprintf(buffer, BUFFER_SIZE, "%s 已退出连接\n", clients[i]->name);
                            broadcast_message(buffer, sd);

                            if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sd, NULL) < 0) {
                                perror("epoll_ctl failed");
                            }

                            close(sd);
                            free(clients[i]);
                            clients[i] = NULL;
                            break;
                        }
                    }
                } else if (valread > 0) {
                    buffer[valread] = '\0';
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i] && clients[i]->socket == sd) {
                            printf("Message from client %s: %s\n", clients[i]->name, buffer);
                            char msg_with_id[BUFFER_SIZE];
                            snprintf(msg_with_id, sizeof(msg_with_id), "%s: ", clients[i]->name);
                            strncat(msg_with_id, buffer, sizeof(msg_with_id) - strlen(msg_with_id) - 1);
                            broadcast_message(msg_with_id, sd);
                            break;
                        }
                    }
                }
            }
        }

        int connected_clients = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i]) connected_clients++;
        }
        printf("Connected clients: %d\n", connected_clients);
    }

    close(server_fd);
    return 0;
}

