
int
mcast_get_if(int sockfd);

int
mcast_get_loop(int sockfd);

int
mcast_get_ttl(int sockfd);

int
mcast_join(int sockfd, const struct sockaddr *grp, socklen_t grplen,
		   const char *ifname, u_int ifindex);

int
mcast_leave(int sockfd, const struct sockaddr *grp, socklen_t grplen);

int
mcast_set_loop(int sockfd, int onoff);

int
mcast_set_ttl(int sockfd, int val);

int
sockfd_to_family(int sockfd);

int
family_to_level(int family);
