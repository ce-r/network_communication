#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h> 
#include <mcast.h>

#define MULTICAST_ADDR "224.1.10.1"
#define START_PORT 8400
#define NUM_PORTS 12
#define IF_NAME "em1"


int main() {
    int sock[NUM_PORTS];
    struct sockaddr_in multicast_addr;
    int i;

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(MULTICAST_ADDR);

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    for (i = 0; i < NUM_PORTS; i++) {
        sock[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock[i] == -1) {
            perror("socket");
            exit(1);
        }

        int reuse = 1;
        if (setsockopt(sock[i], SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        struct sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons(START_PORT + i);

        if (bind(sock[i], (struct sockaddr *) &local_addr, sizeof(local_addr)) == -1) {
            perror("bind");
            exit(1);
        }

        if (mcast_join(sock[i], (struct sockaddr *) &multicast_addr, sizeof(multicast_addr), IF_NAME, 0) == -1) {
            perror("mcast_join");
            exit(1);
        }
    }

    printf("mcast client started. hit enter to exit\n");//add stdin

    while (1) {
        fd_set fds;
        FD_ZERO(&fds);

        int max_fd = -1;
        for (i = 0; i < NUM_PORTS; i++) {
            FD_SET(sock[i], &fds);
            if (sock[i] > max_fd) {
                max_fd = sock[i];
            }
        }

        FD_SET(STDIN_FILENO, &fds);
        if (STDIN_FILENO > max_fd) {
            max_fd = STDIN_FILENO;
        }

        if (select(max_fd + 1, &fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            getchar();
            break;
        }

        for (i = 0; i < NUM_PORTS; i++) {
            if (FD_ISSET(sock[i], &fds)) {
                char buffer[1024];
                ssize_t n = recv(sock[i], buffer, sizeof(buffer), 0);
                if (n == -1) {
                    perror("recv");
                } else if (n > 0) {
                    buffer[n] = '\0';
                    printf("message from port %d: %s\n", START_PORT + i, buffer);
                }
            }
        }
    }

    for (i = 0; i < NUM_PORTS; i++) {
        close(sock[i]);
    }

    return 0;
}