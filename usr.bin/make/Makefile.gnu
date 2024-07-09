# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
# BSDI Makefile.gnu,v 2.3 2002/02/27 16:34:55 benoitp Exp
#

#
# GNU Makefile: used to build pmake for cross development.
#

CP		=	cp -fp
CHMOD		=	chmod
CHMODFLAGS	=	0555

HOST_BIN	=	${WIND_BASE}/host/${WIND_HOST_TYPE}/bin
HOST_LIB	=	${WIND_BASE}/host/${WIND_HOST_TYPE}/lib
WIND_INC	=	${WIND_BASE}/host/include

ifeq (${WIND_HOST_TYPE}, x86-win32)
SUFFIX		=	.exe
LDFLAGS		+=	-L$(HOST_LIB) -lcbsdos
else
SUFFIX		=
endif

ifndef OSNAME
OSNAME		:=	${shell uname}
endif

CFLAGS 		+=	-I. -I$(WIND_INC) -DTORNADO -DOSNAME=\"${OSNAME}\"

SRC1		=	arch.c buf.c compat.c cond.c dir.c for.c \
			hash.c job.c main.c make.c parse.c str.c suff.c \
			targ.c var.c util.c

SRC2		=	lstAppend.c lstAtEnd.c lstAtFront.c lstClose.c \
			lstConcat.c lstDatum.c lstDeQueue.c lstDestroy.c \
			lstDupl.c lstEnQueue.c lstFind.c lstFindFrom.c \
			lstFirst.c lstForEach.c lstForEachFrom.c lstInit.c \
			lstInsert.c lstIsAtEnd.c lstIsEmpty.c lstLast.c \
			lstMember.c lstNext.c lstOpen.c lstRemove.c \
			lstReplace.c lstSucc.c

OBJ1		=	$(patsubst %.c, ${WIND_HOST_TYPE}/%.o, $(SRC1))
OBJ2		=	$(patsubst %.c, ${WIND_HOST_TYPE}/lst.lib/%.o, $(SRC2))

OBJ		=	$(OBJ1) $(OBJ2)

${HOST_BIN}/pmake${SUFFIX}: objdircre ${OBJ}
	${CC} ${WIND_HOST_TYPE}/*.o ${WIND_HOST_TYPE}/lst.lib/*.o $(LDFLAGS) \
		-o ${WIND_HOST_TYPE}/pmake${SUFFIX}
	${CP} ${WIND_HOST_TYPE}/pmake${SUFFIX} ${HOST_BIN}
	${CHMOD} ${CHMODFLAGS} ${HOST_BIN}/pmake${SUFFIX}

${WIND_HOST_TYPE}/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

${WIND_HOST_TYPE}/lst.lib/%.o: lst.lib/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

objdircre:
	@ mkdir -p ${WIND_HOST_TYPE}/lst.lib

clean:
	${RM} -r ./${WIND_HOST_TYPE}
