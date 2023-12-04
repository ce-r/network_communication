#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tcpnitslib.h>

/*
main() implements the fulfillment of request options, including the request 
for an article at the publisher end of the network, using the tcp protocol in the 
unix domain. several methods involve the read() and write() of sockets necessary 
for file transfer and also, string and file path traversal for identification 
of existing files. main() implements the transfer using an iterative approach.
*/

int file_exists(const char* dir, const char* file){
    char filepath[MAXLINE];
    snprintf(filepath, sizeof(filepath), "%s/%s", dir, file);
    return access(filepath, F_OK) != -1;
}

void transfer_file(int sockfd, const char *filepath){
    int fd = open(filepath, O_RDONLY);
    if(fd == -1){
        perror("can't open file\n");
        exit(1);
    }

    char buffer[MAXLINE];
    ssize_t n;
    while((n = read(fd, buffer, sizeof(buffer))) > 0){
        if(write(sockfd, buffer, n) < 0){
            perror("can't read file\n");
            break;
        }
    }
    
    close(fd);
}

void send_list(int sockfd){
    const char* dirs[] =  {"/home/crivas4/674/Articles/", "/home/net_class/674/Articles/"};
    FILE* fp;
    char line[MAXLINE], filepath[MAXLINE];
    int i;

    for (i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++){
        snprintf(filepath, sizeof(filepath), "%s/LIST", dirs[i]);
        fp = fopen(filepath, "r");
        if (fp == NULL) {
            perror("failed to open file list");
        }

        while (fgets(line, sizeof(line), fp) != NULL) {
            if (write(sockfd, line, strlen(line)) != strlen(line)) {
                perror("error writing to socket");
            }
        }

        fclose(fp);
    }
}

void handle_request(int connfd){
    char line[MAXLINE];
    ssize_t n = read(connfd, line, MAXLINE);
    if (n <= 0) {
        return;
    }

    line[n] = '\0';
    if (strcmp(line, "QUIT") == 0){
        printf("quitting publisher...\n");
        exit(1);
        // return;
    } else if (strcmp(line, "LIST") == 0){
        send_list(connfd);
    } else {
        const char *dirs[] = {"/home/crivas4/674/Articles", "/home/net_class/674/Articles"};
        int i, found = 0;
        for (i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++){
            if(file_exists(dirs[i], line)){
                found = 1;
                char actual_fpath[MAXLINE];
                snprintf(actual_fpath, sizeof(actual_fpath), "%s/%s", dirs[i], line);
                transfer_file(connfd, actual_fpath);
                break;
            }
        }
        if (!found) {
            const char *not_found_msg = "file not found\n";
            write(connfd, not_found_msg, strlen(not_found_msg));
        }
    }
    close(connfd);
}

int main(int argc, char *argv[]){
    int pub_fd;

    if (argc > 1){
        fprintf (stderr, "usage: %s\n", argv[0]);
        exit (1);
    }

    if ((pub_fd = setup_publisher (MY_PORT)) == NITS_SOCKET_ERROR){
        fprintf (stderr, "failed to set up the publisher.\n");
        exit(1);
    }

    while (1) {
        int connfd = get_next_subscriber();
        if (connfd != NITS_SOCKET_ERROR){
            handle_request(connfd);  
            close(connfd);
        }
    }

    exit (0);
}