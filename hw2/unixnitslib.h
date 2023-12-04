#ifndef UNIXNITSLIB_H // header guards removed warning
#define UNIXNITSLIB_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/un.h>

#define NITS_SOCKET_ERROR	(-1)
#define NITS_SOCKET_OK		(0)

#define NUM_PUBLISHERS 5
#define NUM_TRIES 5
#define MAX_ATTEMPTS 6
#define MAXLINE 4096
#define MAX_NAME 256
#define ARTICLE_ROOT "/home/net_class/674/Articles"
#define USER_ROOT "/home/crivas4/674/Articles"
#define DISCOVERY_PATH	"/tmp/crivas4_sock00"
#define PUBLISHER_PATH	"/tmp/crivas4_sock3535"
#define SUBSCRIBER_PATH	"/tmp/crivas4_sock44"

// NOTES: 

// ./unix_subscriber2 -d /tmp/crivas4_sock00
// 11/22, had to type in /tmp/crivas4_sock00 manually each time for unix_subscriber2 to work,
// as opposed to using the up arrow for historical commands
// sometimes i didn't but mostly had to, rewrote d_path value assignment to use strcpy
// seems like this behavior is consistent after i quit subscriber2, which kills the publisher

/*

////
[crivas4@absaroka unix_subscriber2]$ ./unix_subscriber2 -d /tmp/crivas4_sock00
unix_subscriber2 started
client address path: /tmp/crivas4_sock44
successfully sent GET_PUB_LIST request

////
[crivas4@absaroka unix_subscriber2]$ ./unix_subscriber2 -d /tmp/crivas4_sock00
unix_subscriber2 started
client address path: /tmp/crivas4_sock44
successfully sent GET_PUB_LIST request
received message from discovery service
list of publishers:
1. publisher path: /tmp/crivas4_sock3535
enter a valid integer from list of publishers: 

*/

/*
 * Discovery service handles this many publishers
*/

typedef enum {GET_PUB_LIST, PUB_LIST, ADVERTISE} DISCOVERY_MESSAGES;

/*
 * message sent from subscriber to discovery service
 */
// #pragma pack(push, 1)
typedef struct {
    DISCOVERY_MESSAGES  msg_type;   /* GET_PUB_LIST */
} disc_get_pub_list;

/*
 * message sent from discovery service to subscriber
 */
typedef struct {
    DISCOVERY_MESSAGES  msg_type;   /* PUB_LIST */
    int num_publishers;
    struct sockaddr_un  address[NUM_PUBLISHERS];
} disc_pub_list;

/*
 * message sent from publisher to discovery service
 */
typedef struct {
    DISCOVERY_MESSAGES  msg_type;       /* ADVERTISE */
    int pubaddr_size;                   /* size of address */
    struct sockaddr_un  pub_address;    /* path of publisher */
} disc_advertise;

typedef union {
    disc_get_pub_list getlist;
    disc_pub_list publist;
    disc_advertise putpub;
} discovery_msgs;

static struct pub_list {
    int valid;
    struct sockaddr_un address; 
} Publishers[NUM_PUBLISHERS];
// #pragma pack(pop)

int setup_subscriber (char *server_path);
int setup_publisher (char *server_path);
int get_next_subscriber ();
int setup_discovery (char * discovery_path);

void set_discovery_fd (int fd);

int advertise_to_discovery (struct sockaddr_un *self_address, struct sockaddr_un *discovery_address);
// bool is_valid_int(int pub_choice, disc_pub_list* pub_list);

#endif // UNIXNITSLIB_H