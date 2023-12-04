/*
 * Nitslib.h
 * Protocol independent Network Information Transfer System
 */

#include <stdbool.h>

#define NITS_SOCKET_ERROR (-1)
#define NITS_SOCKET_OK (0)
/*
 * Discovery service handles this many publishers
*/
#define NUM_PUBLISHERS 5
#define LISTENQ 10

#define USOCK_PREFIX "/unix"

/* typedef enum {GET_PUB_LIST, PUB_LIST, ADVERTISE} DISCOVERY_MESSAGES; */
typedef char DISCOVERY_MESSAGES;
#define GET_PUB_LIST 'G'
#define PUB_LIST 'P'
#define ADVERTISE 'A'

#define ADDRESS_LENGTH	64

// #define MAXLEN (128)
#define MAXLEN 1024
#define MAXLINE 4096

// #define DEFAULT_DISCOVERY ":7640"
#define DEFAULT_PUBLISHER "localhost:8400"
#define DEFAULT_DISCOVERY "localhost:8400"

#define DEFAULT_PORT ":7640"

#define ARTICLE_ROOT "/home/net_class/674/Articles"
#define USER_ROOT "/home/crivas4/674/Articles"

#define DISCOVERY_PATH	"/tmp/crivas4_sock00"
#define PUBLISHER_PATH	"/tmp/crivas4_sock11"
#define SUBSCRIBER_PATH	"/tmp/crivas4_sock22"
#define UN_PREFIX "/unix"

// #define DISCOVERY_IP "128.220.101.247" 
// #define PUBLISHER_IP "128.220.101.234"

// ./discovery -d 128.220.101.247:8406
// ./discovery -d 128.220.101.234:8406

// ./publisher -d 128.220.101.247:8406 -p 128.220.101.247:8407
// ./publisher -d 128.220.101.247:8406 -p 128.220.101.237:8407

// ./publisher -d 128.220.101.234:8406 -p 128.220.101.247:8407
// ./publisher -d 128.220.101.234:8406 -p 128.220.101.234:8407

// ./subscriber2 -d 128.220.101.247:8406
// ./subscriber2 -d 128.220.101.234:8406

// ./discovery -d /unix:/tmp/crivas4_sock00
// ./publisher -d /unix:/tmp/crivas4_sock00 -p /unix:/tmp/crivas4_sock11
// ./subscriber2 -d /unix:/tmp/crivas4_sock00

// rm -rf homework3
// cp -r homework3_absaroka homework3

/*
 * Message sent from discovery service to subscriber
 * Address of publishers is an ascii string.
 */
typedef char ADDRESS[ADDRESS_LENGTH];

/*
 * Message sent from subscriber to discovery service
 */

typedef struct {
	DISCOVERY_MESSAGES  msg_type;	/* GET_PUB_LIST */
} disc_get_pub_list;

/*
 * Address is an ascii string of the form:
 * host:port
 * E.g. for IP absaroka.apl.jhu.edu:4690, for UNIX /unix:/tmp/port7460
 */
typedef struct {
	DISCOVERY_MESSAGES  msg_type;	/* PUB_LIST */
	char num_publishers[3];		/* number of publishers -an ascii string*/
	ADDRESS publisher_address[NUM_PUBLISHERS]; /* an array of characters */
} disc_pub_list;

/*
 * Message sent from publisher to discovery service
 */
typedef struct {
	DISCOVERY_MESSAGES  msg_type;		/* ADVERTISE */
	ADDRESS	pub_address;			/* Address of publisher */
} disc_advertise;

typedef struct {
    int valid;
    char address[ADDRESS_LENGTH]; 
} pub_list;
extern pub_list Publishers[NUM_PUBLISHERS];

typedef union {
	disc_get_pub_list getlist;
	disc_pub_list publist;
	disc_advertise putpub;
} discovery_msgs;

int setup_subscriber (char *host, char *port);
int get_next_subscriber();
int setup_publisher (char *host, char *port);
int setup_discovery (char *host, char *port);
int register_publisher (char *host, char *port, char *dhost, char *dport);
int get_host_and_port (char *hostport, char **host, char **port);
void print_publishers(disc_pub_list *tmp_resp_msg);
