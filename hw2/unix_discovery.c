#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unixnitslib.h>


void print_publishers() {
    printf("list of publishers:\n");
    int i;
    for (i = 0; i < NUM_PUBLISHERS; i++) {
        if (Publishers[i].valid) {
            printf("%d. publisher path: %s", i+1, Publishers[i].address.sun_path);
        }
    }
    printf("\n");
}

int get_pub_list(int fd, struct sockaddr_un cli_addr) {
    printf("get_pub_list function called\n");
    disc_pub_list resp;
    resp.msg_type = PUB_LIST;
    int count = 0, i;

    for (i = 0; i < NUM_PUBLISHERS; i++) {
        if (Publishers[i].valid) {
            resp.address[count++] = Publishers[i].address;
            printf("publisher %d is valid with pathname = %s\n", i, Publishers[i].address.sun_path);
        } else {
            printf("publisher %d is not valid\n", i);
        }
    }
    
    resp.num_publishers = count;
    
    socklen_t addr_len = sizeof(cli_addr.sun_family) + strlen(cli_addr.sun_path) + 1;

    int bytes_sent = sendto(fd, &resp, sizeof(resp), 0, (struct sockaddr*)&cli_addr, addr_len);
    if (bytes_sent < 0) {
        perror("failed to send publisher list");
        return NITS_SOCKET_ERROR;
    }

    printf("sendto - bytes sent: %d\n", bytes_sent);
    return NITS_SOCKET_OK;
}

int add_to_list(struct sockaddr_un advertised_addr) {
    int i;
    for (i = 0; i < NUM_PUBLISHERS; i++) {
        if (Publishers[i].valid && strcmp(Publishers[i].address.sun_path, advertised_addr.sun_path) == 0) {
            printf("error: address already in use, ignoring...\n");
            return NITS_SOCKET_ERROR;
        }
    }

    for (i = 0; i < NUM_PUBLISHERS; i++) {
        if (!Publishers[i].valid) {
            Publishers[i].valid = 1;
            Publishers[i].address = advertised_addr;
            printf("added publisher to the list: %s\n", advertised_addr.sun_path);
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
    struct sockaddr_un cli_addr;
    socklen_t len = sizeof(cli_addr);
    int i;

    if (argc > 1)
    {
        fprintf (stderr, "usage: %s\n", argv[0]);
        exit (1);
    }

    discovery_fd = setup_discovery (DISCOVERY_PATH);
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
        bzero(&cli_addr, sizeof(cli_addr));
        len = sizeof(cli_addr);
        
        int bytes_received = recvfrom(discovery_fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &cli_addr, &len);
        printf("after recvfrom - client address family: %d\n", cli_addr.sun_family);
        printf("after recvfrom - client address path: %s\n", cli_addr.sun_path);
        printf("after recvfrom - client address length: %d\n", len);
        // printf("first 4 bytes received: %02X %02X %02X %02X\n", buffer[0], buffer[1], buffer[2], buffer[3]);

        if (bytes_received < 0){
            perror("recvfrom failure");
            continue;
        }
        printf("received %d bytes.\n", bytes_received);

        memcpy(&my_msg, buffer, bytes_received);

        printf("received a message of type: ");
        switch (my_msg.getlist.msg_type)
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
