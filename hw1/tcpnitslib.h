#include <netinet/in.h>
#define NITS_SOCKET_ERROR   (-1)
#define NITS_SOCKET_OK      (0)
#define MAXLINE 4096
#define MAX_ATTEMPTS 6
#define MY_PORT 8406 //8407 assigned for class

int setup_subscriber (struct in_addr *host, int port);
int setup_publisher (int port);
int get_next_subscriber ();
