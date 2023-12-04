#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
// #include <errno.h> if used here, an error occurs
// error: ‘errno’ undeclared (first use in this function)
#include <unixnitslib.h>

/*

main() implements the menu request options, including the request for an article
at the subscriber end of the network, using the ipc protocol in the unix domain.

*/

bool check_publisher_running(const char* server_path) {
    struct sockaddr_un serv_addr;
    int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("cannot create socket");
        return false;
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_LOCAL;
    strcpy(serv_addr.sun_path, server_path);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection attempt failed");
        close(sockfd);
        return false;
    }

    close(sockfd);
    return true;
}

int main(int argc, char *argv[]){
    struct sockaddr_un host;
    int sockfd;
    char buffer[1024], input[MAXLINE];

    if (argc != 3){
        fprintf(stderr, "usage: %s <command/article> <server path>\n", argv[0]);
        exit(1);
    }

    char *server_path = argv[2];
    
    if (check_publisher_running(server_path)) {
        printf("pub running...\n");
    } else {
        printf("pub not running...\n");
    }

    sockfd = setup_subscriber(server_path);
    printf("socket descriptor: %d\n", sockfd);
    if (sockfd == NITS_SOCKET_ERROR) {
        fprintf(stderr, "invalid socket descriptor\n");
        exit(1);
    }

    strncpy(input, argv[1], sizeof(input) - 1);
    input[sizeof(input) - 1] = '\0';

    if (strlen(input) == 0) {
        close(sockfd);
    }

    if (send(sockfd, input, strlen(input), 0) < 0){
        int errorNumber = errno;
        fprintf(stderr, "failed to send request. error: %s\n", strerror(errorNumber));
        close(sockfd);
        exit(1);
    }

    if (strcmp(input, "QUIT") == 0) {
        printf("quitting...\n");
        close(sockfd);
        exit(1);
    }

    if (strcmp(input, "LIST") == 0) {
        int bytes_received, count = 0;
        printf("available files:\n");
        while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
            write(STDOUT_FILENO, buffer, bytes_received);
        }
        close(sockfd);
    } else {
        int fd = -1;
        int total_bytes = 0, bytes_received;
        while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
            if (bytes_received == strlen("file not found\n")){
                printf("file not found\n");
                close(sockfd);
                exit(1);
            }
            if (fd == -1){
                fd = open(input, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd < 0) {
                    perror("error opening file for writing");
                    close(sockfd);
                    exit(1);
                }
            }

            write(fd, buffer, bytes_received);
            total_bytes += bytes_received;
        }
    
        if (bytes_received < 0) {
            perror("failed to receive response.\n");
        } else {
            printf("received and saved %d bytes to %s\n", total_bytes, input);
        }

        close(fd);
        close(sockfd);
    }

    exit (0);
}
