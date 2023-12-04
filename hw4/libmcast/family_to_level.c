#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <netdb.h>
#include <errno.h>

int
family_to_level(int family)
{
	switch (family) {
	    case AF_INET:
		return IPPROTO_IP;
	    case AF_INET6:
		return IPPROTO_IPV6;
	default:
		return -1;
	}
}
