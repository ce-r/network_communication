#include <netinet/in.h>
#define NITS_SOCKET_ERROR   (-1)
#define NITS_SOCKET_OK      (0)
#define MAXLINE 4096
#define MAX_ATTEMPTS 6
#define MY_PORT 8406 //assigned for class

int setup_subscriber(char *server_path);
int setup_publisher(char *server_path);
int get_next_subscriber ();