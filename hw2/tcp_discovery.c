#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tcpnitslib.h>


void print_publishers() {
    printf("list of publishers:\n");
    char buffer[INET_ADDRSTRLEN];
    int i;
    for (i = 0; i < NUM_PUBLISHERS; i++) {
        if (Publishers[i].valid) {
            inet_ntop(AF_INET, &(Publishers[i].address.sin_addr), buffer, INET_ADDRSTRLEN);
            printf("%d. publisher address: %s, port: %d", i+1, buffer, ntohs(Publishers[i].address.sin_port));
        }
    }
    printf("\n");
}

int get_pub_list (int fd, struct sockaddr_in cli_addr)
{
    printf("get_pub_list function called\n");
    disc_pub_list resp;
    resp.msg_type = PUB_LIST;
    int count = 0, i;
    for (i=0; i < NUM_PUBLISHERS; i++){
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(Publishers[i].address.sin_addr), addr_str, INET_ADDRSTRLEN);
        if (Publishers[i].valid){
            resp.address[count++] = Publishers[i].address;
            printf("publisher %d is valid with IP = %s, port = %d\n", i, addr_str, ntohs(Publishers[i].address.sin_port));
        } else {
            printf("publisher %d is not valid\n", i);
        }
    }
    
    resp.num_publishers = count;

    int bytes_sent = sendto(fd, &resp, sizeof(disc_pub_list), 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));
    if (bytes_sent == -1){
        perror("failed to send publisher list");
        return NITS_SOCKET_ERROR;
    }

    return NITS_SOCKET_OK;
}

int add_to_list (struct sockaddr_in advertised_addr)
{
    int i;
    if (advertised_addr.sin_addr.s_addr == INADDR_LOOPBACK){
        printf("error: localhost address received, ignoring...\n");
        return NITS_SOCKET_ERROR;
    }
    for (i=0; i < NUM_PUBLISHERS; i++){
        if (Publishers[i].valid && memcmp(&Publishers[i].address, &advertised_addr, sizeof(struct sockaddr_in)) == 0) {
            printf("publisher already in the list.\n");
            return NITS_SOCKET_OK; // address already exists
        }
        
        if (!Publishers[i].valid){
            Publishers[i].valid = 1;
            Publishers[i].address = advertised_addr;
            printf("added publisher to the list.\n");
            return NITS_SOCKET_OK;
        }
    }

    printf("failed to add publisher, list full.\n");
    return NITS_SOCKET_ERROR;
}

int main(int argc, char *argv[])
{
    discovery_msgs my_msg;
    int discovery_fd;
    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);
    int i;

    if (argc > 1)
    {
        fprintf (stderr, "usage: %s\n", argv[0]);
        exit (1);
    }

    discovery_fd = setup_discovery (udpDISCOVERY_PORT);// DISCOVERY_PORT
    if (discovery_fd < 0)
    {
        fprintf (stderr, "error in setting up the discovery service.\n");
        exit(1);
    }

    memset(Publishers, 0, sizeof(Publishers));
    for (i = 0; i < NUM_PUBLISHERS; i++)
        Publishers[i].valid = 0;

    while (1)
    {
        printf("waiting for a message from publishers...\n");
        
        char buffer[sizeof(discovery_msgs)];
        bzero(buffer, sizeof(buffer));
        bzero(&cli_addr, sizeof(cli_addr));
        bzero(&my_msg, sizeof(my_msg)); 
        
        int bytes_received = recvfrom(discovery_fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &cli_addr, &len);
        // printf("first 4 bytes received: %02X %02X %02X %02X\n", buffer[0], buffer[1], buffer[2], buffer[3]);
        if (bytes_received < 0){
            perror("recvfrom failure");
            continue;
        }
        printf("received %d bytes.\n", bytes_received);

        memcpy(&my_msg, buffer, bytes_received);

        int received_msg_type = ntohl(my_msg.getlist.msg_type);
        printf("received a message of type: ");
        switch (received_msg_type)
        {
            case GET_PUB_LIST:
                printf("GET_PUB_LIST\n");
                if (bytes_received != sizeof(disc_get_pub_list)) {
                    fprintf(stderr, "incomplete GET_PUB_LIST message received\n");
                    continue;
                }
                get_pub_list (discovery_fd, cli_addr);
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
    exit (0);
}