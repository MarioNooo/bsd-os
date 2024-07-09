#
# Makefile for ikmpd
#

STD_DEFINES = -Wall -Wpointer-arith -DKEY
#CDEBUGFLAGS = -g -DDEBUG 
#CDEBUGFLAGS = -g -DDEBUG -DKEY_DEBUG
CDEBUGFLAGS = -O2 -m486

#
# If you aren't using this for commercial purposes and you have RSAREF,
# then comment out the following defines to allow ISAKMP to use RSA.
# (And put the actual location of RSAREF in there too. It might not be
# in ../rsaref).
#
# Note that if you do this you may have to obtain a license from Cylink
# to use their crypto toolkit.
#
#NONCOMM = -DNON_COMMERCIAL_AND_RSAREF
#RSAINCL = -I../rsaref/include
#RSALIB = ../rsaref/library/rsaref.a

#
# where are the NRL includes
NRLINCL = /usr/inet6/src/sys

INCLUDES = -I. -I../auth -I../cylink/toolkit $(RSAINCL) -I$(NRLINCL)

LOCAL_LIBRARIES = ../cylink/toolkit/libcrypto.a \
                  ../cylink/bignum/libbn.a ../auth/libauth.a \
		  $(RSALIB)

CFLAGS = $(CDEBUGFLAGS) $(INCLUDES) $(STD_DEFINES) $(NONCOMM) 

PROG = ikmpd

CC = gcc

.c.o:
	$(CC) -c $(CFLAGS) $<

SRCS = ikmpd.c initiator.c responder.c cookie.c isadb.c ke.c process.c \
	cert.c sig.c hash.c misc.c utils.c sa_utils.c dh_utils.c sa.c id.c \
	oakley.c nonce.c crypto.c service.c md5.c hmac_md5.c hmac_sha.c \
	header.c notify.c delete.c unix_truerand.c

OBJS = ikmpd.o initiator.o responder.o cookie.o isadb.o ke.o process.o \
	cert.o sig.o hash.o misc.o utils.o sa_utils.o dh_utils.o sa.o id.o \
	oakley.o nonce.o crypto.o service.o md5.o hmac_md5.o hmac_sha.o \
	header.o notify.o delete.o unix_truerand.o

all: ikmpd 

ikmpd:	$(LOCAL_LIBRARIES) $(OBJS)
	$(CC) -o $(PROG) $(LDFLAGS) $(OBJS) $(LOCAL_LIBRARIES)

clean:
	rm -f *.o $(PROG)

