# Copyright (c) 2002 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI Makefile.gnu,v 1.1 2002/02/18 10:58:24 benoitp Exp
#
# GNU Makefile: used to build strip for cross development environment
#

HOST_BIN	=	${WIND_BASE}/host/${WIND_HOST_TYPE}/bin
HOST_LIB	=	${WIND_BASE}/host/${WIND_HOST_TYPE}/lib
WIND_INC	=	${WIND_BASE}/host/include

CP		=	cp -fp
CHMOD		=	chmod
CHMODFLAGS	=	0555

ifeq (${WIND_HOST_TYPE}, x86-win32)
SUFFIX		=	.exe
CFLAGS	   	+=	-g -I$(WIND_INC) -I$(WIND_INC)/bsdos \
			-DBSD_TARGET -DTORNADO
LDFLAGS		+=	-L$(HOST_LIB) -lcbsdos
else
SUFFIX		=
endif

SRCS		=	strip.c strip_elf32.c
OBJS		=	$(patsubst %.c, ${WIND_HOST_TYPE}/%.o, $(SRCS))

PROGRAMS	=	$(HOST_BIN)/bsdstrip$(SUFFIX)

default:	$(PROGRAMS)

$(HOST_BIN)/bsdstrip$(SUFFIX): ${WIND_HOST_TYPE} $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(CHMOD) $(CHMODFLAGS) $@

${WIND_HOST_TYPE}/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

${WIND_HOST_TYPE}:
	@ mkdir -p ${WIND_HOST_TYPE}

rclean clean:
	$(RM) -f $(PROGRAMS)
	$(RM) -rf ./${WIND_HOST_TYPE}
