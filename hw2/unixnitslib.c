#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unixnitslib.h>

#define LISTENQ 5

static int Listen_fd = -1;
static int Discovery_fd = -1;

/*
 * note: host is assumed to be in network byte order.
 * port is in host byte order.
 */

int setup_publisher(char *server_path) {
    int listenfd;
    struct sockaddr_un servaddr;

    listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("error creating socket");
        return NITS_SOCKET_ERROR;
    }

    printf("socket created successfully.\n");

    unlink(server_path);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strncpy(servaddr.sun_path, server_path, sizeof(servaddr.sun_path) - 1);

    printf("attempting to bind the socket...\n");
    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("error binding to socket");
        close(listenfd);  
        return NITS_SOCKET_ERROR;
    } else {
        printf("bind successful.\n");
    }

    printf("setting up to listen...\n");
    if (listen(listenfd, LISTENQ) < 0) {
        perror("error listening on socket");
        close(listenfd);  
        return NITS_SOCKET_ERROR;
    } else {
        printf("listening setup successful.\n");
    }

    Listen_fd = listenfd;  
    return NITS_SOCKET_OK;
}


int setup_subscriber(char *server_path)
{
    int sockfd, i;
    struct sockaddr_un servaddr;

    int retries = 5;
    int delay = 1;

    for (i = 0; i < MAX_ATTEMPTS; i++) {
        sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("error in creating socket");
            continue;
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sun_family = AF_LOCAL;
        strncpy(servaddr.sun_path, server_path, sizeof(servaddr.sun_path) - 1);

        if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == 0) {
            printf("successfully connected to server_path\n");
            return sockfd; 
        } else {
            perror("connect failed");
            close(sockfd); 
        }

        sleep(2);
    }

    if (retries == 0) {
        printf("failed to connect after multiple attempts. exiting...\n");
        return NITS_SOCKET_ERROR;
    }

    return NITS_SOCKET_ERROR;
}

int get_next_subscriber () {
    if (Listen_fd == -1) {
        perror("listen file descriptor is invalid");
        return NITS_SOCKET_ERROR;
    }

    printf("waiting for a new subscriber...\n");

    struct sockaddr_un cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    
    printf("waiting for connections...\n");
    int connfd = accept(Listen_fd, (struct sockaddr*) &cliaddr, &clilen);

    if (connfd < 0) {
        perror("error accepting connection");
        return NITS_SOCKET_ERROR;
    } else {
        printf("accepted a new connection.\n");
    }

    return connfd;
}


void set_discovery_fd (int fd) {
    Discovery_fd = fd;
}

int setup_discovery (char *discovery_path)
{
    int sockfd;
    struct sockaddr_un servaddr;
    sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("failed to create socket");
        return NITS_SOCKET_ERROR;
    }

    unlink(discovery_path);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family      = AF_LOCAL;
    strcpy(servaddr.sun_path, discovery_path);

    if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "error binding to path: %s in setup_discovery\n", discovery_path);
        close(sockfd);
        return NITS_SOCKET_ERROR;
    } else {
        printf("bind successful in setup_discovery\n");
    }
     
    set_discovery_fd(sockfd);
    return sockfd;
}

int advertise_to_discovery(struct sockaddr_un *self_address, struct sockaddr_un *discovery_address){
    int tmp_sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (tmp_sock < 0) {
        perror("failed to create temporary UDP socket");
        return NITS_SOCKET_ERROR;
    }

    discovery_msgs msg;
    memset(&msg, 0, sizeof(msg));
    msg.putpub.msg_type = ADVERTISE;
    msg.putpub.pub_address = *self_address;
    msg.putpub.pubaddr_size = sizeof(*self_address);

    // printf("first 4 bytes to send: %02X %02X %02X %02X\n", ((unsigned char*)&msg)[0], ((unsigned char*)&msg)[1], ((unsigned char*)&msg)[2], ((unsigned char*)&msg)[3]);
    // printf("sending ADVERTISE message of size: %lu bytes.\n", sizeof(msg.putpub));
    // printf("sending ADVERTISE message with msg_type: %d, pubaddr_size: %d, pub_address port: %d\n", ntohl(msg.putpub.msg_type), msg.putpub.pubaddr_size, ntohs(msg.putpub.pub_address.sin_port));

    ssize_t bytes_sent = sendto(tmp_sock, &msg, sizeof(msg.putpub), 0, (struct sockaddr*) discovery_address, sizeof(*discovery_address));
    if (bytes_sent == -1) {
        fprintf(stderr, "error sending ADVERTISE message to path: %s\n", discovery_address->sun_path);
        perror("Discovery_fd is not a valid socket");
        close(tmp_sock);
        return NITS_SOCKET_ERROR;
    } else {
        printf("ADVERTISE message sent successfully (%zd bytes).\n", bytes_sent);
    }

    close(tmp_sock);
    return NITS_SOCKET_OK;
}
