#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unixnitslib.h>

#define LISTENQ 5

/*

unixnitslib.c contains an api allowing us to set up sockets and
establish a connection in the unix domain at the server/client endpoints.

*/

static int global_listenfd = -1;


int setup_publisher(char *server_path){
    int listenfd;
    struct sockaddr_un servaddr;

    listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("error creating socket");
        return NITS_SOCKET_ERROR;
    }

    unlink(server_path);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family      = AF_LOCAL;
    // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // servaddr.sin_port        = htons(port);
    strcpy(servaddr.sun_path, server_path);
    // strncpy(servaddr.sun_path, server_path, sizeof(server_path) - 1);

    //struct sockaddr
    if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        perror("error binding to socket");
        close(listenfd);  
        return NITS_SOCKET_ERROR;
    }

    if (listen(listenfd, LISTENQ) < 0) {//listen(listenfd, 5)
        perror("error listening on socket");
        close(listenfd);  
        return NITS_SOCKET_ERROR;
    }

    global_listenfd = listenfd;  
    return NITS_SOCKET_OK;
}

int setup_subscriber(char *server_path) {
    int sockfd, i;
    struct sockaddr_un servaddr;
    for (i = 0; i < MAX_ATTEMPTS; i++) {
        sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("error in creating socket");
            sleep(2);
            continue;
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sun_family = AF_LOCAL;
        strcpy(servaddr.sun_path, server_path);

        int connResult = connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
        printf("connection result: %d\n", connResult);

        if (connResult == 0) {
            printf("successfully connected to publisher!\n");
            return sockfd;
        } else {
            perror("connect failed");
            close(sockfd);
        }

        sleep(2);
    }

    return NITS_SOCKET_ERROR;
}

int get_next_subscriber() {
    if (global_listenfd == -1) {
        return NITS_SOCKET_ERROR;
    }

    struct sockaddr_un cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int connfd = accept(global_listenfd, (struct sockaddr*) &cliaddr, &clilen);//struct sockaddr of general structure type

    if (connfd < 0) {
        perror("error accepting connection");
        return NITS_SOCKET_ERROR;
    }

    return connfd;
}
