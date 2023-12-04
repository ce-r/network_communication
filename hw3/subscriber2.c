#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <netdb.h>
#include <errno.h>
#include "nitslib.h"

/*
 * Usage: subscriber2 -h -d <discoveryhost>:<discoveryport>
 */

bool is_valid_int(int choice_integer, disc_pub_list* pub_list) {
    int i;
    printf("pub_length: %s, choice_integer: %d\n", pub_list->num_publishers, choice_integer);
    if (choice_integer > 0 && choice_integer<=atoi(pub_list->num_publishers)){
        return true;
    }
    return false;
}


// Just like nitslib.c, we have a one to one relation between last and this project's code, with very few
// exceptions, including the use of libgai and a few functions that allow us to interpret the address
// info from char arrays. I did make a change to an error as noted in my feedback. Printing the list
// of articles before the subscriber interacts with the menu.


int main(int argc, char *argv[]) {
    int c, discovery_sock;
    char *dhost, *dport, *shost, *sport;
    char *discovery = NULL;

    while ((c = getopt(argc, argv, "hd:")) != -1) {
        switch (c) {
            case 'd':
                discovery = optarg;
                break;
            case 'h':
            default:
                fprintf(stderr, "usage: %s -d <host:port>\n", argv[0]);
                exit(1);
        }
    }

    if (discovery == NULL) {
        discovery = malloc(MAXLEN);
        strcpy(discovery, DEFAULT_PORT);
    }

    struct addrinfo dhints, *dres, *dressave;
    memset(&dhints, 0, sizeof(struct addrinfo));
    dhints.ai_family = AF_UNSPEC;
    dhints.ai_socktype = SOCK_DGRAM;

    get_host_and_port(discovery, &dhost, &dport);
    if (getaddrinfo(dhost, dport, &dhints, &dres) != 0) {
        perror("getaddrinfo error");
        exit(1);
    }

    struct addrinfo shints, *sres, *sressave;
    memset(&shints, 0, sizeof(struct addrinfo));
    shints.ai_family = AF_UNSPEC;
    shints.ai_socktype = SOCK_DGRAM;
    
    // get_host_and_port(subscriber, &shost, &sport);
    // shost = UN_PREFIX;
    // sport = SUBSCRIBER_PATH;
    // getaddrinfo(shost, sport, &shints, &sres) 
    if (getaddrinfo(dhost, 0, &shints, &sres) != 0) { // binding to ephemeral port
        perror("getaddrinfo error");
        exit(1);
    }

    dressave = dres;
    sressave = sres;
    do {
        discovery_sock = socket(dres->ai_family, dres->ai_socktype, dres->ai_protocol);
        if (discovery_sock < 0) {
            perror("error in creating socket");
            continue; 
        }
    
        if (bind(discovery_sock, sres->ai_addr, sres->ai_addrlen) == 0) {
            fprintf(stderr, "error binding to port: %d in setup_discovery\n", discovery_sock);
            perror("failed to bind discovery socket");
            close(discovery_sock);
            break; 
        }
        // close(discovery_sock); 
    } while ((dres = dres->ai_next) != NULL && (sres = sres->ai_next) != NULL);

    if (dres == NULL) {
        perror("failed to bind socket");
        freeaddrinfo(dressave);
        exit(1);
    }

    if (sres == NULL) {
        perror("failed to bind socket");
        freeaddrinfo(sressave);
        exit(1);
    } 

    disc_get_pub_list req_msg;
    req_msg.msg_type = GET_PUB_LIST;
    ssize_t bytes_sent = sendto(discovery_sock, &req_msg, sizeof(req_msg), 0, dres->ai_addr, dres->ai_addrlen);
    if (bytes_sent < 0) {
        perror("failed to send GET_PUB_LIST request");
        freeaddrinfo(dressave);
        freeaddrinfo(sressave);
        close(discovery_sock);
        exit(-1);
    }
    printf("successfully sent GET_PUB_LIST request\n");

    disc_pub_list resp_msg;
    ssize_t bytes_received = recvfrom(discovery_sock, &resp_msg, sizeof(resp_msg), 0, NULL, NULL);
    if (bytes_received < 0) {
        perror("error in recvfrom");
        freeaddrinfo(dressave);
        freeaddrinfo(sressave);
        close(discovery_sock);
        exit(-1);
    }
	printf("received message from discovery service\n");

    disc_pub_list resp_msg_cp; 
    resp_msg_cp = resp_msg;
    if (resp_msg.msg_type == PUB_LIST) {
        print_publishers(&resp_msg_cp); 
    } else {
        fprintf(stderr, "unexpected message type received.\n");
    }  

    int pub_choice, s; 
    printf("enter a valid integer from list of publishers: ");
    if (scanf("%d", &pub_choice) != 1) {
        printf("no integer input.\n");
        exit(1);
    }

    if (!is_valid_int(pub_choice, &resp_msg_cp)){
        printf("invalid integer.\n");
        exit(1);
    }

    int pc = pub_choice - 1;
    while ((s = getchar()) != '\n' && c != EOF);//clear stdin

    if(resp_msg.num_publishers > 0){    
        char buffer0[1024];
        const char* request0 = "LIST";
        char *pre_phost, *pre_pport;
        resp_msg_cp = resp_msg;

        get_host_and_port(resp_msg_cp.publisher_address[pc], &pre_phost, &pre_pport);
        int subscriber_sock0 = setup_subscriber(pre_phost, pre_pport);
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
			char *phost, *pport;
            resp_msg_cp = resp_msg;
			
            get_host_and_port(resp_msg_cp.publisher_address[pc], &phost, &pport);
            int subscriber_sock = setup_subscriber(phost, pport);
            // int subscriber_sock = setup_subscriber("128.220.101.234", "8407");
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

	freeaddrinfo(dressave);
    freeaddrinfo(sressave);
    close(discovery_sock);
    exit(0);	
}
