#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>

#define PORT "3150"
#define MAXSIZEDATA 1024
#define IPv4_SERVER_ADDRESS "192.168.100.249"

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int open_connection_with_server(void){

    int sockfd, status;

    char IPaddr[INET6_ADDRSTRLEN];

    struct addrinfo hints, *serverinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(IPv4_SERVER_ADDRESS, PORT, &hints, &serverinfo)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    for (p = serverinfo; p != NULL; p = p->ai_next){

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (sockfd == -1) {
            perror("client: socket");
            continue;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), IPaddr, sizeof IPaddr);
        printf("client: attempting connection to: %s on port %s \n", IPaddr, PORT);

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL){
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), IPaddr, sizeof IPaddr);
    printf("client: connected to %s \n", IPaddr);

    freeaddrinfo(serverinfo);

   return sockfd;
}

int receive_data_from_server(int sockfd, char *buffer){

    int num_bytes = recv(sockfd, buffer, MAXSIZEDATA - 1, 0);

    if (num_bytes == -1){
        perror("recv");
        return -1;
    }

    buffer[num_bytes] =  '\0';

    printf("%s \n", buffer);

    return 1;
}

int send_data_to_server(int sockfd, const char *buffer){

    ssize_t bytes_sent;

    bytes_sent = send(sockfd, buffer, strlen(buffer), 0);

    if (bytes_sent == -1){
        perror("send");
        return -1;
    }

    return 0;
}

int main(void){

    int sockfd = -1;

    char send_buffer[MAXSIZEDATA];
    char receive_buffer[MAXSIZEDATA];

    char cmd[10];

    printf("TCP client running... \n");

    while(true){

        printf("> ");
        fgets(cmd, sizeof(cmd), stdin);
        cmd[strcspn(cmd, "\n")] = '\0';

        if (strcmp(cmd, "open") == 0){

            sockfd = open_connection_with_server();

            if (sockfd != -1){

                printf("client: sent 220 \n");

                strcpy(send_buffer, "220\n");

                send_data_to_server(sockfd, send_buffer);

                receive_data_from_server(sockfd, receive_buffer);
                continue;
            }
        }
        
        else if (strcmp(cmd, "send") == 0){

            if (sockfd == -1){
                printf("No connection established. \n");
                continue;
            }

            printf("client: sent 330 \n");

            strcpy(send_buffer, "330\n");

            send_data_to_server(sockfd, send_buffer);

            receive_data_from_server(sockfd, receive_buffer);

            printf("type message: ");
            fgets(send_buffer, sizeof(send_buffer), stdin);

            send_data_to_server(sockfd, send_buffer);

            receive_data_from_server(sockfd, receive_buffer);
            continue;
        }

        else if (strcmp(cmd, "close") == 0){

            if (sockfd != -1){

                 printf("client: sent 550 \n");

                strcpy(send_buffer, "550\n");
                
                send_data_to_server(sockfd, send_buffer);

                receive_data_from_server(sockfd, receive_buffer);

                close(sockfd);
                sockfd = -1;
                continue;
            }

            printf("No connection established. \n");
            continue;
        }

        else if (strcmp(cmd, "exit") == 0){

            if (sockfd != -1){
                close(sockfd);
                printf("goodbye! \n");
                break;
            }

            printf("goodbye! \n");

            break;
        }

        else {
            printf("invalid command. \n");
            continue;
        }
    }   
}