#
# Project:	Network Information Transfer System (NITS)
# Module:	
# Programmer:	John C. Noble
# Organization:	JHU EP
# Host System:	SunOS 4
# Date Created: February 10
# Modifications:
#	$Log$
# 
# 

# Library to make (eg: libeg.a)
LIBRARY = libgai.a 


# Source files for LIBRARY routines
SRC	= freeaddrinfo.c \
	ga_aistruct.c \
	ga_clone.c \
	ga_echeck.c \
	ga_nsearch.c \
	ga_port.c \
	ga_serv.c \
	ga_unix.c \
	gai_strerror.c \
	getaddrinfo.c \
	getnameinfo.c \
	gn_ipv46.c \
	sock_ntop.c

# Master include files for libraries.
include ../../../Make_templates/Make.lib

testga: testga.o libgai.a
	cc testga.o libgai.a -o testga
