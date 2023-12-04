#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <tcpnitslib.h>

// #define SUB_BUFLEN 80
// #define ART_BUFLEN 1024
#define ARTICLE_ROOT "/home/net_class/674/Articles"
#define USER_ROOT "/home/crivas4/674/Articles"

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
        return -1;
    } else if (strcmp(line, "CHECK_DIR") == 0) {
        handle_dir_request(connfd);
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

char* get_local_ip() {
    int sockfd;
    struct sockaddr_in local_addr;
    struct sockaddr_in server_addr;
    char* local_ip = malloc(INET_ADDRSTRLEN);

    // create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
        perror("socket creation failed");
        free(local_ip);
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53); // DNS port
    inet_pton(AF_INET, "8.8.8.8", &server_addr.sin_addr); // google's public DNS IP address

    // connect the socket, doesn't actually establish a connection, just selects the interface
    if(connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed");
        free(local_ip);
        close(sockfd);
        return NULL;
    }

    // get the local address of the socket
    socklen_t addr_len = sizeof(local_addr);
    if(getsockname(sockfd, (struct sockaddr *) &local_addr, &addr_len) == -1) {
        perror("getsockname failed");
        free(local_ip);
        close(sockfd);
        return NULL;
    }

    // convert the IP to string format
    inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, INET_ADDRSTRLEN);

    close(sockfd);
    return local_ip;
}

void handle_sigchld(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[])
{
    char *ip;
    int sub_fd;
    struct sockaddr_in self_address, discovery_address;
    
    if (argc > 3)
    {
        fprintf (stderr, "usage: <-d discovery_service_address> %s\n", argv[0]);
        exit (1);
    }
    
    if (argc == 3 && strcmp(argv[1], "-d") == 0) {
        ip = argv[2];
    } else if (argc == 1) {
        ip = DISCOVERY_IP;
    } else {
        exit(1);
    }

    if ((setup_publisher (PUBLISHER_PORT)) == NITS_SOCKET_ERROR)
    {
        fprintf (stderr, "error in setting up the publisher.\n");
        exit(1);
    }

    char* pub_ip = get_local_ip();
    printf("pub IP address: %s\n", pub_ip);

    self_address.sin_family = AF_INET;
    self_address.sin_port = htons(PUBLISHER_PORT);
    if (!inet_aton(pub_ip, &self_address.sin_addr)) {
        perror("inet_aton(pub_ip...) failed");
        exit(1);
    }
    free(pub_ip);

    discovery_address.sin_family = AF_INET;
    discovery_address.sin_port = htons(udpDISCOVERY_PORT);//DISCOVERY_PORT
    if (!inet_aton(ip, &discovery_address.sin_addr)) {
        perror("inet_aton(ip...) failed");
        exit(1);
    }
    
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
            int status = handle_request(sub_fd);
            close(sub_fd);
            if (status == -1) {
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
