#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <strings.h>
#include <fcntl.h>
#include <nitslib.h>

static int Listen_fd = -1;
static int Discovery_fd = -1;

pub_list Publishers[NUM_PUBLISHERS];


// These utils are responsible for creating sockets and communications networks in a protocol independent
// manner. It is practically the same code and approach with the exception of creating a linked list of 
// addrinfo structures to iterate over, which allows us to be protocol independent. 


int setup_publisher(char *host, char *port) {
    printf("calling setup_publisher %s %s\n", host, port);
    
    struct addrinfo hints, *res, *ressave;
    int listenfd = -1, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM; 

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo error");
        return NITS_SOCKET_ERROR;
    }

    ressave = res;
    do {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0) {
            perror("error in creating socket");
            continue; 
        }

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0) {
            fprintf(stderr, "error binding to port: %d in setup_publisher\n", listenfd);
            perror("failed to bind in setup_publisher");
            break;
        }

        close(listenfd); 
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) {
        perror("setup_publisher failed");
        freeaddrinfo(ressave);
        return NITS_SOCKET_ERROR;
    }

    if (listen(listenfd, LISTENQ) < 0) {
        perror("listen error");
        close(listenfd);
        return NITS_SOCKET_ERROR;
    }

    printf("setup_publisher successful.\n");
    freeaddrinfo(ressave);
    Listen_fd = listenfd;
    return listenfd;
}

int setup_subscriber(char *host, char *port) {
    printf("calling setup_subscriber %s %s\n", host, port);
    
    struct addrinfo hints, *res, *ressave;
    int sockfd = -1, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo error");
        return NITS_SOCKET_ERROR;
    }

    ressave = res;  
    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0){
            perror("error in creating socket");
            continue;
        }

        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0){
            perror("failed to connect");
            break;
        }  

        close(sockfd);
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) {
        perror("connect failed");
        freeaddrinfo(ressave);
        return NITS_SOCKET_ERROR;
    }

    freeaddrinfo(ressave);
    return sockfd;
}

int get_next_subscriber() {
    printf("calling get_next_subscriber\n");

    if (Listen_fd == -1) {
        perror("listen file descriptor is invalid");
        return NITS_SOCKET_ERROR;
    }

    printf("waiting for a new subscriber...\n");

    struct sockaddr_storage cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    printf("waiting for connections...\n");
    int connfd = accept(Listen_fd, (struct sockaddr *) &cliaddr, &clilen);
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

int setup_discovery(char *host, char *port) {
    printf("calling setup_discovery %s %s\n", host, port);
    
    struct addrinfo hints, *res, *ressave;
    int sockfd = -1, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; 

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo error");
        return NITS_SOCKET_ERROR;
    }

    ressave = res;
    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) {
            perror("error in creating socket");
            continue; 
        }

        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
            fprintf(stderr, "error binding to port: %d in setup_discovery\n", sockfd);
            perror("failed to bind in setup_discovery");
            break;
        }

        close(sockfd);
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) {
        perror("setup_discovery failed");
        freeaddrinfo(ressave);
        return NITS_SOCKET_ERROR;
    }

    printf("discovery setup successful.\n");
    freeaddrinfo(ressave);
    set_discovery_fd(sockfd);
    return sockfd;
}

/*
 * Send message to discovery service
 * register host, port to discovery service on dhost, dport
 */
int register_publisher(char *host, char *port, char *dhost, char *dport) {
	printf ("calling register_publisher w/ pubhost: %s, pubport: %s, dhost: %s, dport: %s\n", host, port, dhost, dport);
    
    int tmp_sock;
    struct addrinfo hints, *ressave, *dres;
    struct sockaddr_storage daddr_storage;
    struct sockaddr *daddr = (struct sockaddr *) &daddr_storage;
    socklen_t daddr_len;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(dhost, dport, &hints, &dres) != 0) {
        perror("getaddrinfo error for discovery");
        return NITS_SOCKET_ERROR;
    }

    memcpy(daddr, dres->ai_addr, dres->ai_addrlen);
    daddr_len = dres->ai_addrlen;
    ressave = dres;
    
    tmp_sock = socket(dres->ai_family, dres->ai_socktype, dres->ai_protocol);
    if (tmp_sock < 0) {
        perror("failed to create udp socket");
        freeaddrinfo(ressave);
        return NITS_SOCKET_ERROR;
    }

    discovery_msgs msg;
    memset(&msg, 0, sizeof(msg));
    msg.putpub.msg_type = ADVERTISE;
    snprintf(msg.putpub.pub_address, ADDRESS_LENGTH, "%s:%s", host, port);

    ssize_t bytes_sent = sendto(tmp_sock, &msg, sizeof(msg), 0, dres->ai_addr, dres->ai_addrlen);
    if (bytes_sent == -1) {
        perror("error sending message to discovery service");
        freeaddrinfo(ressave);
        close(tmp_sock);
        return NITS_SOCKET_ERROR;
    } else {
        printf("ADVERTISE sent successfully (%zd bytes).\n", bytes_sent);
    }

    freeaddrinfo(ressave);
    close(tmp_sock);
    return NITS_SOCKET_OK;
}

/*
 * Translates server:port to separate server and host pointers.
 * Note *host and *port must be writable strings (not constants)!
 */
int get_host_and_port (char *hostport, char **host, char **port)
{
	if (*hostport == ':')
	{
		*host = NULL;
		*port = &hostport[1];
		return(0);
	}

	*host = strtok(hostport, ":");
	*port = strtok(NULL, ":");
}

void print_publishers(disc_pub_list *pub_list) {
    int i;
    int num_publishers = atoi(pub_list->num_publishers);
    
    printf("list of publishers:\n");
    for (i = 0; i < num_publishers; ++i) {
        char *host, *port;
        char tmp_pubaddr[ADDRESS_LENGTH]; 

        strncpy(tmp_pubaddr, pub_list->publisher_address[i], ADDRESS_LENGTH - 1);
        tmp_pubaddr[ADDRESS_LENGTH - 1] = '\0';  

        get_host_and_port(tmp_pubaddr, &host, &port);

        if (strncmp(pub_list->publisher_address[i], "/unix:", 6) == 0) {
            printf("%d. unix file path: %s\n", i + 1, port);
        } else if (strchr(host, ':') != NULL) {
            printf("%d. ipv6 address: %s, port: %s\n", i + 1, host, port);
        } else {
            printf("%d. ipv4 address: %s, port: %s\n", i + 1, host, port);
        }
    }
}
