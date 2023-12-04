#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MULTICAST_ADDR "224.1.10.1"
#define DEFAULT_PORT 8400

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in multicast_addr;
    int port = DEFAULT_PORT;

    if (argc == 2) {
        port = atoi(argv[1]);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(MULTICAST_ADDR);
    multicast_addr.sin_port = htons(port);

    char message[128];
    while (1) {
        // char message[128];
        // snprintf(message, sizeof(message), "mcast message from server: %d", rand());
        snprintf(message, sizeof(message), "%d", rand());

        printf("%s\n", message);

        if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *) &multicast_addr, sizeof(multicast_addr)) == -1) {
            perror("sendto");
        }

        sleep(5);
    }

    // while (1) {
    //     printf("submit message (hit enter to quit): ");
    //     if (fgets(message, sizeof(message), stdin) == NULL) {
    //         perror("fgets");
    //         break;  
    //     }

    //     size_t len = strlen(message);
    //     if (len > 0 && message[len - 1] == '\n') {
    //         message[len - 1] = '\0';
    //     }

    //     if (strlen(message) == 0) {
    //         break;
    //     }

    //     if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *) &multicast_addr, sizeof(multicast_addr)) == -1) {
    //         perror("sendto");
    //     }

    //     sleep(5);
    // }
    close(sockfd);

    return 0;
}