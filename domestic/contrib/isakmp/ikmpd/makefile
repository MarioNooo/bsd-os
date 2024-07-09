#
# Makefile for ikmpd
#
BINDIR= /usr/sbin
BINMODE= 511
MAN8=ikmpd.0

STD_DEFINES = -Wall -Wpointer-arith
STD_DEFINES += -DKEY2
#CDEBUGFLAGS = -g -DDEBUG -DKEY -DKEY_DEBUG
#CDEBUGFLAGS = -g -DDEBUG -DROUTER_ONLY
#CDEBUGFLAGS = -O2 -m486

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
#RSALIB = ${.CURDIR}/../rsaref/library/rsaref.a

INCLUDES = -I${.CURDIR}/. -I${.CURDIR}/../auth \
	-I${.CURDIR}/../cylink/toolkit ${RSAINCL}

.if exists(${.CURDIR}/../cylink/toolkit/obj)
LIB_ADD += ${.CURDIR}/../cylink/toolkit/obj/libcrypto.a
.else
LIB_ADD += ${.CURDIR}/../cylink/toolkit/libcrypto.a
.endif
.if exists(${.CURDIR}/../cylink/bignum/obj)
LIB_ADD += ${.CURDIR}/../cylink/bignum/obj/libbn.a
.else
LIB_ADD += ${.CURDIR}/../cylink/bignum/libbn.a
.endif
.if exists(${.CURDIR}/../auth/obj)
LIB_ADD += ${.CURDIR}/../auth/obj/libauth.a
.else
LIB_ADD += ${.CURDIR}/../auth/libauth.a
.endif

LDADD += ${LIB_ADD} ${RSALIB}
DPADD += ${LIB_ADD} ${RSALIB}

CFLAGS += ${CDEBUGFLAGS} ${INCLUDES} ${STD_DEFINES} ${NONCOMM} 

PROG = ikmpd

CC = gcc

SRCS = ikmpd.c initiator.c responder.c cookie.c isadb.c ke.c process.c \
	cert.c sig.c hash.c misc.c utils.c sa_utils.c dh_utils.c sa.c id.c \
	oakley.c nonce.c crypto.c service.c md5.c hmac_md5.c hmac_sha.c \
	header.c notify.c delete.c unix_truerand.c

.include <bsd.prog.mk>
