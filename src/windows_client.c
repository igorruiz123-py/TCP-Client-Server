#define _WIN32_WINNT 0x0601

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT "3150"
#define MAXSIZEDATA 1024
#define IPv4_SERVER_ADDRESS "192.168.100.249"

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

SOCKET open_connection_with_server(void) {

    SOCKET sockfd = INVALID_SOCKET;
    int status;

    char IPaddr[INET6_ADDRSTRLEN];

    struct addrinfo hints, *serverinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(IPv4_SERVER_ADDRESS, PORT, &hints, &serverinfo);
    if (status != 0) {
        printf("getaddrinfo failed: %d\n", status);
        return INVALID_SOCKET;
    }

    for (p = serverinfo; p != NULL; p = p->ai_next) {

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == INVALID_SOCKET) {
            printf("socket failed: %d\n", WSAGetLastError());
            continue;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr),
                  IPaddr, sizeof IPaddr);

        printf("client: attempting connection to %s on port %s\n", IPaddr, PORT);

        if (connect(sockfd, p->ai_addr, (int)p->ai_addrlen) == SOCKET_ERROR) {
            printf("connect failed: %d\n", WSAGetLastError());
            closesocket(sockfd);
            sockfd = INVALID_SOCKET;
            continue;
        }

        break;
    }

    freeaddrinfo(serverinfo);

    if (sockfd == INVALID_SOCKET) {
        printf("client: failed to connect\n");
        return INVALID_SOCKET;
    }

    printf("client: connected successfully\n");
    return sockfd;
}

int receive_data_from_server(SOCKET sockfd, char *buffer) {

    int num_bytes = recv(sockfd, buffer, MAXSIZEDATA - 1, 0);

    if (num_bytes == SOCKET_ERROR) {
        printf("recv failed: %d\n", WSAGetLastError());
        return -1;
    }

    if (num_bytes == 0) {
        printf("server closed connection\n");
        return -1;
    }

    buffer[num_bytes] = '\0';
    printf("%s", buffer);

    return 1;
}

int send_data_to_server(SOCKET sockfd, const char *buffer) {

    int bytes_sent = send(sockfd, buffer, (int)strlen(buffer), 0);

    if (bytes_sent == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        return -1;
    }

    return 0;
}

int main(void) {

    WSADATA wsa;
    SOCKET sockfd = INVALID_SOCKET;

    char send_buffer[MAXSIZEDATA];
    char receive_buffer[MAXSIZEDATA];
    char cmd[16];

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    printf("TCP client running (Windows)...\n");

    while (true) {

        printf("> ");
        fgets(cmd, sizeof(cmd), stdin);
        cmd[strcspn(cmd, "\n")] = '\0';

        if (strcmp(cmd, "open") == 0) {

            sockfd = open_connection_with_server();

            if (sockfd != INVALID_SOCKET) {
                strcpy(send_buffer, "220\n");
                send_data_to_server(sockfd, send_buffer);
                receive_data_from_server(sockfd, receive_buffer);
            }
        }

        else if (strcmp(cmd, "send") == 0) {

            if (sockfd == INVALID_SOCKET) {
                printf("No connection established.\n");
                continue;
            }

            strcpy(send_buffer, "330\n");
            send_data_to_server(sockfd, send_buffer);
            receive_data_from_server(sockfd, receive_buffer);

            printf("type message: ");
            fgets(send_buffer, sizeof(send_buffer), stdin);
            send_data_to_server(sockfd, send_buffer);
            receive_data_from_server(sockfd, receive_buffer);
        }

        else if (strcmp(cmd, "exit") == 0) {

            if (sockfd != INVALID_SOCKET) {
                strcpy(send_buffer, "550\n");
                send_data_to_server(sockfd, send_buffer);
                receive_data_from_server(sockfd, receive_buffer);
                closesocket(sockfd);
            }

            break;
        }

        else {
            printf("invalid command\n");
        }
    }

    WSACleanup();
    return 0;
}
