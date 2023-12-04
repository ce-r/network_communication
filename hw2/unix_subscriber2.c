#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <unixnitslib.h>


void print_publishers(disc_pub_list* pub_list) {
    printf("list of publishers:\n");
    int i;
    for (i = 0; i < pub_list->num_publishers; i++) {
        printf("%d. publisher path: %s\n", i+1, pub_list->address[i].sun_path);
    }
}

bool is_valid_int(int choice_integer, disc_pub_list* pub_list) {
    int i;
    printf("pub_length: %d, choice_integer: %d\n", pub_list->num_publishers, choice_integer);
    for (i=1; i<=pub_list->num_publishers; i++){
        if (choice_integer == i){    
            return true;
        }
    }

    return false;
}

int check_dir_exists(struct sockaddr_un *address) {
    int sockfd;
    char buffer[MAXLINE];
    char request[] = "CHECK_DIR";

    sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("error opening socket");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *) address, sizeof(*address)) < 0) {
        perror("error connecting");
        close(sockfd);
        return -1;
    }

    send(sockfd, request, strlen(request), 0);
    int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
    buffer[bytes_received] = '\0';
    printf("received response: %s\n", buffer); 

    close(sockfd);

    if (strstr(buffer, "dir exists")) {
        return 1;
    } else {
        return 0;
    }
}

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


int main(int argc, char *argv[])
{
    printf("unix_subscriber2 started\n");
    
    char *d_path;
    struct sockaddr_un serv_addr;
    int discovery_sock;
    disc_get_pub_list req_msg;
    disc_pub_list resp_msg;
    ssize_t bytes_received;

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (argc > 3)
    {
        fprintf(stderr, "usage: %s <optional -d discovery_service_address>\n", argv[0]);
        exit (-1);
    }

    if (argc == 3 && strcmp(argv[1], "-d") == 0) {
        d_path = argv[2];
    } else if (argc == 1) {
        d_path = DISCOVERY_PATH;
    } else {
        exit(1);
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_LOCAL;
    strcpy(serv_addr.sun_path, d_path);

    discovery_sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(discovery_sock < 0){
        perror("failed to setup discovery service");
        exit (-1);
    }

    struct sockaddr_un cli_addr;
    bzero(&cli_addr, sizeof(cli_addr));
    cli_addr.sun_family = AF_LOCAL;
    strcpy(cli_addr.sun_path, SUBSCRIBER_PATH);

    unlink(SUBSCRIBER_PATH);
    if (bind(discovery_sock, (struct sockaddr*) &cli_addr, sizeof(cli_addr)) < 0) {
        perror("bind failed");
        close(discovery_sock);
        exit(-1);
    }
    printf("client address path: %s\n", cli_addr.sun_path);  

    req_msg.msg_type = GET_PUB_LIST;
    ssize_t bytes_sent = sendto(discovery_sock, &req_msg, sizeof(req_msg), 0, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if (bytes_sent < 0) {
        perror("failed to send GET_PUB_LIST request");
        close(discovery_sock);
        exit(-1);
    }
    printf("successfully sent GET_PUB_LIST request\n");

    bytes_received = recvfrom(discovery_sock, &resp_msg, sizeof(resp_msg), 0, NULL, NULL);
    if (bytes_received < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            fprintf(stderr, "error in recvfrom: timed out waiting for a response\n");
        } else {
            perror("error in recvfrom");
        }
        close(discovery_sock);
        exit(-1);
    }
    printf("received message from discovery service\n");

    if (resp_msg.msg_type == PUB_LIST) {
        print_publishers(&resp_msg); 
    } else {
        fprintf(stderr, "unexpected message type received.\n");
    }  

    int pub_choice, c; 
    printf("enter a valid integer from list of publishers: ");
    if (scanf("%d", &pub_choice) != 1) {
        printf("no integer input.\n");
        exit(1);
    }

    if (!is_valid_int(pub_choice, &resp_msg)){
        printf("invalid integer.\n");
        exit(1);
    }

    int pc = pub_choice - 1;
    while ((c = getchar()) != '\n' && c != EOF);


    // char buffer0[1024];
    // const char* request0 = "LIST";
    // printf("selected publisher: %s\n", resp_msg.address[pc].sun_path);

    // int subscriber_sock0 = setup_subscriber(resp_msg.address[pc].sun_path);
    // if (subscriber_sock0 < 0){
    //     perror("failed to setup subscriber0");
    //     close(subscriber_sock0);
    //     exit(1);
    // } 

    // if (send(subscriber_sock0, request0, strlen(request0), 0) < 0){
    //     fprintf(stderr, "failed to send request\n");
    //     close(subscriber_sock0);
    //     exit(1);
    // }   

    // if (strcmp(request0, "LIST") == 0) {
    //     int bytes_received;
    //     printf("available files:\n");
    //     while ((bytes_received = recv(subscriber_sock0, buffer0, sizeof(buffer0), 0)) > 0) {
    //         write(STDOUT_FILENO, buffer0, bytes_received);
    //     }
    //     close(subscriber_sock0);
    // }

    if(resp_msg.num_publishers > 0){   
        // CODE ABOVE BELONGS HERE //
        char buffer0[1024];
        const char* request0 = "LIST";
        printf("selected publisher: %s\n", resp_msg.address[pc].sun_path);

        int subscriber_sock0 = setup_subscriber(resp_msg.address[pc].sun_path);
        if (subscriber_sock0 < 0){
            perror("failed to setup subscriber0");
            close(subscriber_sock0);
            exit(1);
        } 

        if (send(subscriber_sock0, request0, strlen(request0), 0) < 0){
            fprintf(stderr, "failed to send request\n");
            close(subscriber_sock0);
            exit(1);
        }   

        if (strcmp(request0, "LIST") == 0) {
            int bytes_received;
            printf("available files:\n");
            while ((bytes_received = recv(subscriber_sock0, buffer0, sizeof(buffer0), 0)) > 0) {
                write(STDOUT_FILENO, buffer0, bytes_received);
            }
            close(subscriber_sock0);
        }

        while(1){
            char buffer[1024], input[MAXLINE];
            // printf("selected publisher: %s\n", resp_msg.address[pc].sun_path);

            int subscriber_sock = setup_subscriber(resp_msg.address[pc].sun_path);
            if (subscriber_sock < 0){
                perror("failed to setup subscriber");
                close(subscriber_sock);
                exit(1);
            } else {
                printf("connected to publisher successfully\n");
                printf("enter your request (QUIT, or file name): ");
                
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = '\0';

                if (strlen(input) == 0) {
                    close(subscriber_sock);
                }

                if (send(subscriber_sock, input, strlen(input), 0) < 0){
                    fprintf(stderr, "failed to send request\n");
                    close(subscriber_sock);
                    exit(1);
                }   

                if (strcmp(input, "QUIT") == 0) {
                    printf("quitting...\n");
                    close(subscriber_sock);
                    exit(1);
                } else {
                    int fd = -1;
                    int total_bytes = 0, bytes_received;
                    while ((bytes_received = recv(subscriber_sock, buffer, sizeof(buffer), 0)) > 0) {
                        if (bytes_received == strlen("file not found\n")){
                            printf("file not found\n");
                            close(subscriber_sock);
                            exit(1);
                        }
                        if (fd == -1){
                            fd = open(input, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                            if (fd < 0) {
                                perror("error opening file for writing");
                                close(subscriber_sock);
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
                    close(subscriber_sock);
                }
            }
        }
    }

    close(discovery_sock);
    exit (0);
}