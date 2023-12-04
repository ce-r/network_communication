#include <netinet/in.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <ifaddrs.h>
#include <unixnitslib.h>

#define SUB_BUFLEN 80
#define ART_BUFLEN 1024

// static int quit_status = 0;


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

void handle_dir_request(int connfd) {
    const char *dirs[] = {USER_ROOT, ARTICLE_ROOT};
    int i, found = 0;
    for (i=0; i<sizeof(dirs) / sizeof(dirs[0]); i++) {
        if(access(dirs[i], F_OK) != -1){
            found = 1;
            break;
        }
    }

    if (found) {
        char response[] = "dir exists";
        printf("sending confirmation that directory exists.\n");
        write(connfd, response, strlen(response));
    } else {
        char response[] = "dir not found\n";
        printf("dir not found. sending error message.\n");
        write(connfd, response, strlen(response));
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

int main(int argc, char *argv[])
{
    char *pub_path;
    int sub_fd;
    struct sockaddr_un self_address, discovery_address;
    
    if (argc > 3)
    {
        fprintf (stderr, "usage:  %s\n", argv[0]);
        exit (1);
    }
    
    if (argc == 3 && strcmp(argv[1], "-d") == 0) {
        pub_path = argv[2];
    } else if (argc == 1) {
        pub_path = PUBLISHER_PATH;
    } else {
        exit(1);
    }

    if ((setup_publisher (pub_path)) == NITS_SOCKET_ERROR)
    {
        fprintf (stderr, "error in setting up the publisher.\n");
        exit(1);
    }

    bzero(&self_address, sizeof(self_address));
    self_address.sun_family = AF_LOCAL;
    strcpy(self_address.sun_path, pub_path);

    bzero(&discovery_address, sizeof(discovery_address));
    discovery_address.sun_family = AF_LOCAL;
    strcpy(discovery_address.sun_path, DISCOVERY_PATH);
    
    int advert_sock = advertise_to_discovery(&self_address, &discovery_address);
    
    printf("advertise result: %d\n", advert_sock);
    if (advert_sock != NITS_SOCKET_OK){
        perror("failed to advertise to discovery service");
        exit(1);
    }

    if (signal(SIGCHLD, handle_sigchld) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    
    while (1)
    {
        sub_fd = get_next_subscriber();
        if (sub_fd == NITS_SOCKET_ERROR)
        {
            // exit (1);
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {               /* child */
            int quit_status = handle_request(sub_fd);
            close(sub_fd);
            printf("quit status: %d\n", quit_status);
            if (quit_status == -1) {
                kill(getppid(), SIGTERM); /* send termination signal to parent */
            }
            exit(0);                  /* exiting the child */
        } else if (pid == -1) {
            perror("fork");           /* can't create child */
            exit(-1);                 /* exit the publisher if we can't fork */
        }
        close (sub_fd);
    }

    exit (0);
}