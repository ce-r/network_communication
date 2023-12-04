
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include "mcastlib.h"

int
mcast_leave(int sockfd, const struct sockaddr *grp, socklen_t grplen)
{
	struct group_req req;
	req.gr_interface = 0;
	if (grplen > sizeof(req.gr_group)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(&req.gr_group, grp, grplen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),
			MCAST_LEAVE_GROUP, &req, sizeof(req)));
}

int
mcast_leave_source_group(int sockfd, const struct sockaddr *src, socklen_t srclen,
					const struct sockaddr *grp, socklen_t grplen)
{
	struct group_source_req req;
	req.gsr_interface = 0;
	if (grplen > sizeof(req.gsr_group) || srclen > sizeof(req.gsr_source)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(&req.gsr_group, grp, grplen);
	memcpy(&req.gsr_source, src, srclen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),
			MCAST_LEAVE_SOURCE_GROUP, &req, sizeof(req)));
}
