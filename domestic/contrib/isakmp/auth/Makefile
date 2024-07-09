#
# Makefile for authentication routines and app library
#

MAN8=gen_dss_key.0
MLINKS+=gen_dss_key.8 add_pub_key.8
MLINKS+=gen_dss_key.8 dump_key.8
MLINKS+=gen_dss_key.8 list_keys.8
MLINKS+=gen_dss_key.8 fingerprint.8
MLINKS+=gen_dss_key.8 add_preshr_key.8
MLINKS+=gen_dss_key.8 show_preshr.8
BINDIR=/usr/sbin

STD_DEFINES = -Wall -Wpointer-arith
#CDEBUGFLAGS = -g -DDEBUG
CDEBUGFLAGS = -O2

.if ${MACHINE} == "i386"
CDEBUGFLAGS+=	-m486
.endif

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
#RSALIB = ../rsaref/library/rsaref.a
#RSAINCL = -I../rsaref/include
#RSAKEY = gen_rsa_key


INCLUDES = -I${.CURDIR}/. -I${.CURDIR}/../cylink/toolkit $(RSAINCL) 

CFLAGS = $(CDEBUGFLAGS) $(INCLUDES) $(STD_DEFINES) $(NONCOMM)

LIB_AUTH = libauth.a 

.if exists(${.CURDIR}/../cylink/toolkit/obj)
LIB_CYLINK+= ${.CURDIR}/../cylink/toolkit/obj/libcrypto.a
.else
LIB_CYLINK+= ${.CURDIR}/../cylink/toolkit/libcrypto.a
.endif
.if exists(${.CURDIR}/../cylink/bignum/obj)
LIB_CYLINK+= ${.CURDIR}/../cylink/bignum/obj/libbn.a
.else
LIB_CYLINK+= ${.CURDIR}/../cylink/bignum/libbn.a
.endif


CC = gcc

.c.o:
	$(CC) -c $(CFLAGS) $<

SRC1 = authlib.c
OBJ1 = authlib.o

SRC2 = add_pub_key.c 
OBJ2 = add_pub_key.o

SRC3 = dump_key.c
OBJ3 = dump_key.o

SRC4 = gen_dss_key.c 
OBJ4 = gen_dss_key.o

SRC5 = list_keys.c 
OBJ5 = list_keys.o

SRC6 = fingerprint.c 
OBJ6 = fingerprint.o

SRC7 = gen_rsa_key.c
OBJ7 = gen_rsa_key.o

SRC8 = add_preshr_key.c
OBJ8 = add_preshr_key.o

SRC9 = show_preshr.c
OBJ9 = show_preshr.o

CMDS= add_pub_key dump_key gen_dss_key list_keys fingerprint \
	add_preshr_key show_preshr $(RSAKEY)

all:  libauth $(CMDS) $(MAN8)

libauth:	$(OBJ1)
	$(AR) rc $(LIB_AUTH) $(OBJ1)
	$(RANLIB) $(LIB_AUTH)

add_pub_key:	$(LIB_AUTH) $(OBJ2)
	$(CC) -o  add_pub_key $(LDFLAGS) $(OBJ2) $(LIB_AUTH) 

dump_key:	$(LIB_AUTH) $(OBJ3)
	$(CC) -o  dump_key $(LDFLAGS) $(OBJ3) $(LIB_AUTH) 

gen_dss_key:	$(LIB_AUTH) $(LIB_CYLINK) $(OBJ4)
	$(CC) -o  gen_dss_key $(LDFLAGS) $(OBJ4) $(LIB_AUTH) $(LIB_CYLINK)

gen_rsa_key:	$(RSALIB) $(OBJ7)
	$(CC) -o  gen_rsa_key $(LDFLAGS) $(OBJ7) $(RSALIB)

list_keys:	$(LIB_AUTH) $(OBJ5)
	$(CC) -o  list_keys $(LDFLAGS) $(OBJ5) $(LIB_AUTH) 

fingerprint:	$(LIB_AUTH) $(LIB_CYLINK) $(OBJ6)
	$(CC) -o  fingerprint $(LDFLAGS) $(OBJ6) $(LIB_AUTH) $(LIB_CYLINK)

add_preshr_key:	$(LIB_AUTH) $(LIB_CYLINK) $(OBJ8)
	$(CC) -o  add_preshr_key $(LDFLAGS) $(OBJ8) $(LIB_AUTH) $(LIB_CYLINK)

show_preshr: 	$(LIB_AUTH) $(LIB_CYLINK) $(OBJ9)
	$(CC) -o  show_preshr $(LDFLAGS) $(OBJ9) $(LIB_AUTH) $(LIB_CYLINK)

clean:
	rm -f *.o $(LIB_AUTH) $(MAN8) add_pub_key dump_key \
		gen_dss_key list_keys fingerprint gen_rsa_key \
		add_preshr_key show_preshr

beforeinstall: $(CMDS) $(MAN8)
	for i in $(CMDS); do \
        install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
            ${INSTALLFLAGS} $$i ${DESTDIR}${BINDIR}; \
	done

.include "bsd.prog.mk"

cleandir: clean
