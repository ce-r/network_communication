#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <ifaddrs.h>
#include <errno.h>
#include "nitslib.h"

#define SUB_BUFLEN 80
#define ART_BUFLEN 1024
// #define ARTICLE_ROOT "/home/jcn/674/Articles"

static int Verbose = 0;


// Just like nitslib.c, we have a one to one relation between last and this project's code, with very few
// exceptions, including the use of libgai. The approach here is essentially the same as last project, 
// from the code structure to the functions. I made some changes from the feedback in last project, 
// including the addition of a call to exit when fopen fails and killing the parent process, when 
// subscriber2 quits. 


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
    const char* dirs[] =  {USER_ROOT, ARTICLE_ROOT};
    FILE* fp;
    char line[MAXLINE], filepath[MAXLINE];
    int i;

    for (i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++){
        snprintf(filepath, sizeof(filepath), "%s/LIST", dirs[i]);
        fp = fopen(filepath, "r");
        if (fp == NULL) {
            perror("failed to open file list");
            exit(-1);
        }

        while (fgets(line, sizeof(line), fp) != NULL) {
            if (write(sockfd, line, strlen(line)) != strlen(line)) {
                perror("error writing to socket");
            }
        }

        fclose(fp);
    }
}

int handle_request(int connfd){
    char line[MAXLINE];
    ssize_t n = read(connfd, line, MAXLINE);
    if (n <= 0) {
        return -1;
    }

    line[n] = '\0';
    if (strcmp(line, "QUIT") == 0){
        printf("quitting publisher...\n");
        close(connfd);
        // exit(1);
        return -1;
    } else if (strcmp(line, "LIST") == 0){
        send_list(connfd);
    } else {
        const char *dirs[] = {USER_ROOT, ARTICLE_ROOT};
        int i, found = 0;
        for (i=0; i<sizeof(dirs) / sizeof(dirs[0]); i++) {
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
    return 0;
}

void handle_sigchld(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    int c;
    char publisher[MAXLEN];
    char discovery[MAXLEN];
    char *phost, *pport;
    char *dhost, *dport;

    strncpy(publisher, DEFAULT_PUBLISHER, MAXLEN);
    strncpy(discovery, DEFAULT_DISCOVERY, MAXLEN);

    while ((c = getopt(argc, argv, "hvp:d:")) != -1) {
        switch (c) {
            case 'v':
                Verbose = 1;
                break;
            case 'p':
                strncpy(publisher, optarg, MAXLEN);
                break;
            case 'd':
                strncpy(discovery, optarg, MAXLEN);
                break;
            case 'h':
            default:
                fprintf(stderr, "usage: %s -d <host:port> -p <host:port>\n", argv[0]);
                exit(1);
        }
    }

    if (Verbose) {
        printf("publisher: %s\n", publisher);
        printf("discovery: %s\n", discovery);
    }

    get_host_and_port(publisher, &phost, &pport);
    get_host_and_port(discovery, &dhost, &dport);

    if (setup_publisher(phost, pport) == NITS_SOCKET_ERROR) {
        fprintf(stderr, "cannot set up publisher.\n");
        exit(1);
    }

    if (register_publisher(phost, pport, dhost, dport) != NITS_SOCKET_OK) {
        perror("failed to register with discovery service");
        exit(1);
    }

    if (signal(SIGCHLD, handle_sigchld) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    while (1)
    {
        int sub_fd = get_next_subscriber();
        if (sub_fd == NITS_SOCKET_ERROR)
        {
            // exit (1);
            continue;
        }
        
        pid_t pid = fork();
        if (pid == 0) {                   /* child */
            int status = handle_request(sub_fd);
            close(sub_fd);
            if (status == -1) {
                kill(getppid(), SIGTERM); /* send termination signal to parent */
            }
            exit(0);                      /* exiting the child */
        } else if (pid == -1) {
            perror("fork");               /* can't create child */
            exit(-1);                     /* exit the publisher if we can't fork */
        }
        close (sub_fd);
    }

    exit(0);
}
