#ifndef TCPNITSLIB_H // header guards removed warning
#define TCPNITSLIB_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define NITS_SOCKET_ERROR -1
#define NITS_SOCKET_OK 0
#define PUBLISHER_PORT 8406 //7641 changed in tcp_publisher, worked 10/26 3:05, tcp_subscriber2 doesn't work now
#define DISCOVERY_PORT 8407 // 7640 //8406 can't use 7640 because absaroka has a firewall on it
#define udpDISCOVERY_PORT 8408 // Don't use your assigned port to bind to the local UDP socket for the sending of 
                               // the ADVERTISE message from the publisher to the discovery service.  
                               // That port should be only bound to a UDP socket in the discovery service.
#define TCP_SUBSCRIBER_PORT 7642
#define TCP_SUBSCRIBER2_PORT 7643 
#define MY_PORT 8406 //assigned for class
#define NUM_PUBLISHERS 5
#define MAX_ATTEMPTS 6
#define MAXLINE 4096
#define DISCOVERY_IP "128.220.101.247" //234 changed 10/29 1:20am //247
#define PUBLISHER_IP "128.220.101.234" 

/*
 * discovery service handles this many publishers
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
    struct sockaddr_in  address[NUM_PUBLISHERS];
} disc_pub_list;

/*
 * message sent from publisher to discovery service
 */
typedef struct {
    DISCOVERY_MESSAGES  msg_type;       /* ADVERTISE */
    int pubaddr_size;                   /* size of address */
    struct sockaddr_in  pub_address;    /* address of publisher */
} disc_advertise;

typedef union {
    disc_get_pub_list getlist;
    disc_pub_list publist;
    disc_advertise putpub;
} discovery_msgs;

static struct pub_list {
    int valid;
    struct sockaddr_in address; 
} Publishers[NUM_PUBLISHERS];
// #pragma pack(pop)

int setup_subscriber (struct in_addr *host, int port);
int setup_publisher (int port);
int setup_discovery (int discovery_port);

void set_discovery_fd(int fd);

int get_next_subscriber ();

// discovery_msgs get_next_discovery_msg ();

int advertise_to_discovery (struct sockaddr_in *self_address, struct sockaddr_in *discovery_address);
// bool is_valid_int(int pub_choice, disc_pub_list* pub_list);

#endif // TCPNITSLIB_H