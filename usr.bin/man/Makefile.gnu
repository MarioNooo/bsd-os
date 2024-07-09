# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
# 
# BSDI Makefile.gnu,v 1.1 2002/04/23 11:37:39 benoitp Exp
#
 
#
# GNU Makefile: 	used to build and install config in a cross development
#			environment
#

CP		=	cp -fp
RM		=	rm -f
CHMOD		=	chmod
CHMODFLAGS	=	0555

HOST_BIN	=       ${WIND_BASE}/host/${WIND_HOST_TYPE}/bin

ifeq (${WIND_HOST_TYPE}, x86-win32)
HOST_LIB	=	-L${WIND_BASE}/host/${WIND_HOST_TYPE}/lib -lcbsdos
HOST_INCLUDE	=	-I. -I${WIND_BASE}/host/include \
			-I${WIND_BASE}/host/include/bsdos
else
HOST_LIB	=	-lc
HOST_INCLUDE	=	-I.
endif


ifeq (${WIND_HOST_TYPE}, x86-win32)
SUFFIX  	=       .exe
else
SUFFIX		=
endif

SRCS		=   	config.c man.c
CFLAGS		+=	-DTORNADO ${HOST_INCLUDE} -g
CLEANFILES	=	
OBJS		=	$(patsubst %.c, ${WIND_HOST_TYPE}/%.o, $(SRCS))

$(HOST_BIN)/bsdman$(SUFFIX): objdircre $(OBJS)
	${CC} ${WIND_HOST_TYPE}/*.o -o ${WIND_HOST_TYPE}/bsdman ${HOST_LIB}
	$(CP) ${WIND_HOST_TYPE}/bsdman$(SUFFIX) $(HOST_BIN)
	$(CHMOD) $(CHMODFLAGS) $@

${WIND_HOST_TYPE}/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

objdircre:
	@ mkdir -p ${WIND_HOST_TYPE}

clean:
	${RM} -rf $(HOST_BIN)/bsdman$(SUFFIX) ./${WIND_HOST_TYPE} $(CLEANFILES)
