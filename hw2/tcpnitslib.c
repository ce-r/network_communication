#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <fcntl.h>
#include <tcpnitslib.h>

static int Listen_fd;
static int Discovery_fd;

/*
 * note: host is assumed to be in network byte order.
 * port is in host byte order.
 */

struct Publisher {
    char ip[16]; // assuming IPv4
    int port;
};

// bool is_service_listening(const char* ip, int port) {
//     int sockfd;
//     struct sockaddr_in serv_addr;

//     sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sockfd < 0) {
//         perror("error opening socket");
//         return false;
//     }

//     serv_addr.sin_family = AF_INET;
//     serv_addr.sin_addr.s_addr = inet_addr(ip);
//     serv_addr.sin_port = htons(port);

//     if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
//         close(sockfd);
//         return false;
//     }

//     close(sockfd);
//     return true;
// }

int setup_publisher(int port)
{
    printf ("setting up tcp publisher on %d\n", port);
    int listenfd, optv = 1;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("error creating socket");
        return NITS_SOCKET_ERROR;
    }

    // setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optv, sizeof(optv));
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optv, sizeof(optv)) == -1) {
            perror("setsockopt");
            exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(port);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "error binding to port: %d\n", port);
        perror("bind in setup_publisher failed");
        close(listenfd);  
        return NITS_SOCKET_ERROR;
    }

    if (listen(listenfd, 10) < 0) {
        fprintf(stderr, "error listening on port: %d\n", port);
        perror("listen failed");
        close(listenfd);  
        return NITS_SOCKET_ERROR;
    }

    Listen_fd = listenfd;  
    return NITS_SOCKET_OK;
}

int setup_subscriber(struct in_addr *host, int port)
{
    int sockfd, i, optv;
    struct sockaddr_in servaddr;

    struct Publisher pub;

    strncpy(pub.ip, inet_ntoa(*host), 15);
    pub.ip[15] = '\0';
    pub.port = port;

    // if (!is_service_listening(pub.ip, pub.port)){
    //     printf("discovery/publisher not running or listening on %s:%d\n", pub.ip, pub.port);
    //     return NITS_SOCKET_ERROR;
    // }

    int retries = 5;
    int delay = 1;

    for (i = 0; i < MAX_ATTEMPTS; i++) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("error in creating socket");
            continue;
        }

        int optval = 1;
        // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optv, sizeof(optv)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        servaddr.sin_addr = *host;

        if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) != 0) {
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(servaddr.sin_addr), ip_str, INET_ADDRSTRLEN);
            fprintf(stderr, "error connecting to IP: %s port: %d\n", ip_str, ntohs(servaddr.sin_port));
            sleep(delay);
            delay *= 2; 
            retries--;

            close(sockfd);
            continue;
        } else {
            return sockfd;
        }

        close(sockfd);
    }

    if (retries == 0) {
        printf("failed to connect after multiple attempts. exiting...\n");
        return NITS_SOCKET_ERROR;
    }

    return NITS_SOCKET_ERROR;
}

/*
 * port is in host byte order
 */

int get_next_subscriber ()
{
    if (Listen_fd == -1) {
        perror("listen file descriptor is invalid");
        return NITS_SOCKET_ERROR;
    }

    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    printf("waiting for connections...\n");
    int connfd = accept(Listen_fd, (struct sockaddr*) &cliaddr, &clilen);
    if (connfd < 0) {
        perror("error accepting connection");
        return NITS_SOCKET_ERROR;
    } 
    printf("accepted a new connection.\n");

    return connfd;
}

void set_discovery_fd (int fd) {
    Discovery_fd = fd;
}

int setup_discovery (int discovery_port)
{
    int sockfd;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("failed to create socket");
        return NITS_SOCKET_ERROR;
    }

    int opt = 1;
    // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(discovery_port);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "error binding to port: %d in setup_discovery\n", discovery_port);
        perror("failed to bind in setup_discovery");
        close(sockfd);
        return NITS_SOCKET_ERROR;
    }
    printf("bind successful in setup_discovery\n");
     
    set_discovery_fd(sockfd);
    return sockfd;
}

int advertise_to_discovery(struct sockaddr_in *self_address, struct sockaddr_in *discovery_address){
    int tmp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (tmp_sock < 0) {
        perror("failed to create temporary UDP socket");
        return NITS_SOCKET_ERROR;
    }

    if (bind(tmp_sock, (struct sockaddr *) self_address, sizeof(*self_address)) < 0) {//ephemeral port goes away
        perror("bind failed in advertise_to_discovery");
        close(tmp_sock);
        return NITS_SOCKET_ERROR;
    }

    discovery_msgs msg;
    memset(&msg, 0, sizeof(msg));
    msg.putpub.msg_type = htonl(ADVERTISE);
    msg.putpub.pub_address = *self_address;
    msg.putpub.pubaddr_size = sizeof(*self_address);

    ssize_t bytes_sent = sendto(tmp_sock, &msg, sizeof(msg.putpub), 0, (struct sockaddr*) discovery_address, sizeof(*discovery_address));
    if (bytes_sent == -1) {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(discovery_address->sin_addr), ip_str, INET_ADDRSTRLEN);
        fprintf(stderr, "error sending ADVERTISE message to IP: %s port: %d\n", ip_str, ntohs(discovery_address->sin_port));
        perror("Discovery_fd is not a valid socket");
        close(tmp_sock);
        return NITS_SOCKET_ERROR;
    }
    printf("ADVERTISE message sent successfully (%zd bytes).\n", bytes_sent);

    close(tmp_sock);
    return NITS_SOCKET_OK;
}

// bool is_valid_int(int choice_integer, disc_pub_list* pub_list) {
//     int i;
//     printf("pub_length: %d, choice_integer: %d\n", pub_list->num_publishers, choice_integer);
//     for (i=1; i<=pub_list->num_publishers; i++){
//         if (choice_integer == i){    
//             return true;
//         }
//     }

//     return false;
// }