/*
 *	Discovery service
 *
 * Usage : discovery <host>:<port>
 *	       discovery :<port>
 * If host not supplied, use ""
 * If port not supplied, use DEFAULT_DISCOVERY (:<your port>)
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <netdb.h>
#include <fcntl.h>
#include "nitslib.h"

// Just like nitslib.c, we have a one to one relation between last and this project's code, with very few
// exceptions, including the use of libgai and a few functions that allow us to interpret the address
// info from char arrays. 

int get_pub_list(int fd, char *host, char *port) {
    printf("get_pub_list function called\n");
    
    disc_pub_list resp;
    resp.msg_type = PUB_LIST;
    int count = 0, i;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_DGRAM; 

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo error in get_pub_list");
        return NITS_SOCKET_ERROR;
    }

    for (i = 0; i < NUM_PUBLISHERS; i++) {
        if (Publishers[i].valid) {
            strncpy(resp.publisher_address[count], Publishers[i].address, ADDRESS_LENGTH - 1);
            resp.publisher_address[count][ADDRESS_LENGTH - 1] = '\0';
            printf("resp.publisher_address: %s\n", resp.publisher_address);
            printf("Publishers[%d].address: %s\n", i, Publishers[i].address);
            count++;
        }
    }
    snprintf(resp.num_publishers, sizeof(resp.num_publishers), "%d", count);

    int bytes_sent = sendto(fd, &resp, sizeof(resp), 0, res->ai_addr, res->ai_addrlen);
    if (bytes_sent < 0) {
        perror("failed to send publisher list");
        freeaddrinfo(res);
        return NITS_SOCKET_ERROR;
    }

    printf("sendto - bytes sent: %d\n", bytes_sent);
    freeaddrinfo(res);
    return NITS_SOCKET_OK;
}

int add_to_list(ADDRESS advertised_addr) {
    char *host, *port;
    printf("advertised publisher: %s\n", advertised_addr);
    get_host_and_port(advertised_addr, &host, &port);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; 

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo error in add_to_list");
        return NITS_SOCKET_ERROR;
    }

    int i;
    for (i = 0; i < NUM_PUBLISHERS; i++) {
        if (!Publishers[i].valid) {
            Publishers[i].valid = 1;
            snprintf(Publishers[i].address, ADDRESS_LENGTH, "%s:%s", host, port);
            Publishers[i].address[ADDRESS_LENGTH - 1] = '\0';
            
            printf("added publisher to the list: %s\n", Publishers[i].address);
            freeaddrinfo(res);
            return NITS_SOCKET_OK;
        }
    }

    printf("failed to add publisher, list full.\n");
    freeaddrinfo(res);
    return NITS_SOCKET_ERROR;
}

int main(int argc, char *argv[]) {
    char *host, *port;
    char discovery[MAXLEN];
    int discovery_fd;
    int c, i;
    struct sockaddr_storage cli_addr_storage;
    struct sockaddr *cli_addr = (struct sockaddr *) &cli_addr_storage;
    socklen_t len = sizeof(cli_addr_storage);

    /*
	 * Make sure discovery is pointing to a writable string.
	 */
    strcpy(discovery, DEFAULT_DISCOVERY);

    while ((c = getopt(argc, argv, "d:h")) != -1) {
        switch (c) {
            case 'h':
                fprintf(stderr, "usage: %s -d <host:port>\n", argv[0]);
                exit(0);
            case 'd':
                strncpy(discovery, optarg, MAXLEN);
                break;
            default:
                fprintf(stderr, "usage: %s -d <host:port>\n", argv[0]);
                exit(1);
        }
    }

    get_host_and_port(discovery, &host, &port);

    if ((discovery_fd = setup_discovery(host, port)) == NITS_SOCKET_ERROR) {
        fprintf(stderr, "setup discovery failed.\n");
        exit(1);
    }

    discovery_msgs my_msg;
    memset(&my_msg, 0, sizeof(my_msg));

    memset(Publishers, 0, sizeof(Publishers));
    for (i = 0; i < NUM_PUBLISHERS; i++){
        Publishers[i].valid = 0;
        strcpy(Publishers[i].address, "");
    }

    while (1) {
        printf("waiting for a message from publishers...\n");
        
        char buffer[sizeof(discovery_msgs)];
        bzero(buffer, sizeof(buffer));
        bzero(cli_addr, sizeof(cli_addr_storage));
        len = sizeof(cli_addr_storage);

        int bytes_received = recvfrom(discovery_fd, buffer, sizeof(buffer), 0, cli_addr, &len);
        if (bytes_received < 0) {
            perror("recvfrom failure");
            continue;
        }
        printf("received %d bytes.\n", bytes_received);

        memcpy(&my_msg, buffer, bytes_received);

        printf("received a message of type: ");
        switch (my_msg.getlist.msg_type)
        {
            case GET_PUB_LIST:
                printf("GET_PUB_LIST\n");
                if (bytes_received != sizeof(disc_get_pub_list)) {
                    fprintf(stderr, "incomplete GET_PUB_LIST message received\n");
                    continue;
                }

                printf("before getnameinfo\n");
                char subhost[NI_MAXHOST], subport[NI_MAXSERV];
                int status = getnameinfo(cli_addr, len, subhost, NI_MAXHOST, subport, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
                if (status != 0) {
                    fprintf(stderr, "getnameinfo failed: %s\n", gai_strerror(status));
                    continue;
                }
                printf("after getnameinfo\n");
                printf("subhost: %s, subport: %s\n", subhost, subport);

                get_pub_list (discovery_fd, subhost, subport);
                break;
            case ADVERTISE:
                printf("ADVERTISE\n");
                if (bytes_received < sizeof(disc_advertise)){
                    fprintf(stderr, "incomplete ADVERTISE message received\n");
                    continue;
                }
                printf("expected bytes for ADVERTISE: %lu, received: %d\n", sizeof(disc_advertise), bytes_received);

                int result = add_to_list (my_msg.putpub.pub_address);
                if (result == NITS_SOCKET_OK) {
                    printf("successfully added to publisher list.\n");
                } else {
                    printf("failed to add to publisher list.\n");
                }
                break;
            default:
                printf("DEFAULT\n");
                fprintf(stderr, "discovery service: bad message type\n");
        }
    }

    close(discovery_fd);
    exit(0);
}
