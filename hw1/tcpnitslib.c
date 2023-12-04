#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <tcpnitslib.h>

/*
tcpnitslib.c contains an api allowing us to set up sockets and establish a connection
in the tcp/ip domain at the server/client endpoints. 
*/

static int global_listenfd = -1; 

int setup_publisher(int port){
    printf ("setting up tcp publisher on %d\n", port);
    
    int listenfd;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        return NITS_SOCKET_ERROR;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        close(listenfd);  
        return NITS_SOCKET_ERROR;
    }

    if (listen(listenfd, 5) < 0) {
        close(listenfd);  
        return NITS_SOCKET_ERROR;
    }

    global_listenfd = listenfd;  

    return NITS_SOCKET_OK;
}

int setup_subscriber(struct in_addr *pub_host, int pub_port) {
    int sockfd, i;
    struct sockaddr_in servaddr;
    for (i = 0; i < MAX_ATTEMPTS; i++) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("error in creating socket");
            continue; // retry next iteration
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(pub_port);
        servaddr.sin_addr = *pub_host;

        if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == 0) {
            return sockfd;
        }

        close(sockfd);

        if (i < MAX_ATTEMPTS - 1) {
            sleep(2);// delay for 2 seconds before the next attempt
        }
    }

    return NITS_SOCKET_ERROR;
}

int get_next_subscriber() {
    if (global_listenfd == -1) {
        return NITS_SOCKET_ERROR;
    }

    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int connfd = accept(global_listenfd, (struct sockaddr*) &cliaddr, &clilen);

    if (connfd < 0) {
        perror("error accepting connection");
        return NITS_SOCKET_ERROR;
    }

    return connfd;
}