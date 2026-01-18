#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <time.h>

#define PORT "3150"
#define BACKLOG 1
#define MAXSIZEDATA 1024

/*
CODES MEANING:

220 -> open connections succesfully
250 -> server message confirmation of client message received
330 -> client message received successfully
500 -> error
550 -> client disconnected
*/

int recv_line(int sockfd, char *buffer, size_t maxlen) {
    size_t i = 0;
    char c;

    while (i < maxlen - 1) {
        ssize_t n = recv(sockfd, &c, 1, 0);
        if (n <= 0) return n;
        if (c == '\n') break;
        buffer[i++] = c;
    }

    buffer[i] = '\0';
    return i;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void printtime(time_t t, char *buffer, size_t size){

    struct tm *tme = localtime(&t);

    strftime(buffer, size, "%d/%m/%Y - %H:%M:%S", tme);
}

int handle_client(int newsockfd, FILE *file, char *IPaddr) {

    struct sockaddr_storage their_addr;

    char buffer[MAXSIZEDATA];

    if (recv_line(newsockfd, buffer, MAXSIZEDATA) <= 0)
        return -1;

    if (strcmp(buffer, "220") != 0) {
        send(newsockfd, "server: 500 invalid start code\n", 31, 0);
        return -1;
    }

    send(newsockfd, "server: 220 OK\n", 15, 0);

    while (true) {

        if (recv_line(newsockfd, buffer, MAXSIZEDATA) <= 0)
            break;

        if (strcmp(buffer, "330") == 0) {

            send(newsockfd, "server: 330 ready\n", 18, 0);

            if (recv_line(newsockfd, buffer, MAXSIZEDATA) <= 0)
                break;

            printf("server: client message '%s'\n", buffer);
            fprintf(file, "client message: %s\n", buffer);
            fflush(file);

            send(newsockfd, "server: 250 OK\n", 15, 0);
        }
        else if (strcmp(buffer, "550") == 0) {

            send(newsockfd, "server: 550 OK\n", 15, 0);
            printf("server: client %s disconnected from server \n", IPaddr);
            break;
        }
    }

    return 1;
}

int main(void){

    srand((time(NULL)));

    int sockfd;

    int status;

    int binding;

    int listening;

    int newsockfd;

    struct addrinfo hints, *serverinfo, *p;

    struct sockaddr_storage their_addr;

    char buffer[64];

    socklen_t sin_size;

    char IPaddr[INET6_ADDRSTRLEN];

    FILE *file;

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &hints, &serverinfo)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    for (p = serverinfo; p != NULL; p = p->ai_next){

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (sockfd == -1){
            perror("server: socket");
            continue;
        }

        binding = bind(sockfd, p->ai_addr, p->ai_addrlen);

        if (binding == -1){
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(serverinfo);

    if (p == NULL){
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    listening = listen(sockfd, BACKLOG);

    if (listening == -1){
        perror("listen");
        exit(1);
    }

    printf("server: listening for connections... \n");

    file = fopen("connections.log", "a");

    if (file == NULL){
        perror("open");
        exit(1);
    }

    while(true){

        time_t t = time(NULL);

        sin_size = sizeof their_addr;

        newsockfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        if (newsockfd == -1){
            perror("accept");
            exit(1);
        }

        printtime(t, buffer, sizeof(buffer));

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), IPaddr, sizeof IPaddr);

        printf("server: got connection from %s on port %s at %s \n", IPaddr, PORT, buffer);

        fprintf(file, "client connected from %s on port %s at %s \n", IPaddr, PORT, buffer);
        fflush(file);

        handle_client(newsockfd, file, IPaddr);

        close (newsockfd);

        fprintf(file, "----------------------------------------------------------- \n");
        fflush(file);

    }

    fclose(file);
    close(sockfd);

    return 0;
}


