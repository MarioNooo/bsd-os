# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
# 
# BSDI Makefile.gnu,v 2.4 2002/04/25 09:58:59 didier Exp
#
 
#
# GNU Makefile: 	used to build and install config in a cross development
#			environment
#

HOST_BIN	=       ${WIND_BASE}/host/${WIND_HOST_TYPE}/bin
ifeq (${WIND_HOST_TYPE}, x86-win32)
HOST_LIB	=	-L${WIND_BASE}/host/${WIND_HOST_TYPE}/lib -lcbsdos
HOST_INCLUDE	=	-I. -I${WIND_BASE}/host/include/bsdos
else
HOST_LIB	=	-lc
HOST_INCLUDE	=	-I.
endif

YFLAGS		=	-d -v
CP		=	cp -fp
RM		=	rm -f
SED		=	sed
MV		=	mv
CHMOD		=	chmod
CHMODFLAGS	=	0555

ifeq (${WIND_HOST_TYPE}, x86-win32)
SUFFIX  	=       .exe
else
SUFFIX		=
endif

SRCS		=   	files.c gram.c mgram.c hash.c main.c mkheaders.c \
			mkioconf.c mkmakefile.c mkswap.c mkswgeneric.c  \
			pack.c sem.c template.c util.c variable.c \
			mscan.c cscan.c
CFLAGS		+=	-DTORNADO -DCCDEBUG -DMMDEBUG ${HOST_INCLUDE}
CLEANFILES	=	gram.c mgram.c cscan.c mscan.c cc.tab.h mm.tab.h \
			cc.output mm.output
OBJS		=	$(patsubst %.c, ${WIND_HOST_TYPE}/%.o, $(SRCS))

$(HOST_BIN)/config$(SUFFIX): objdircre $(OBJS)
	${CC} ${WIND_HOST_TYPE}/*.o -o ${WIND_HOST_TYPE}/config ${HOST_LIB}
	$(CP) ${WIND_HOST_TYPE}/config$(SUFFIX) $(HOST_BIN)
	$(CHMOD) $(CHMODFLAGS) $@

${WIND_HOST_TYPE}/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

gram.c:	gram.y
	${YACC} $(YFLAGS) $<
	${SED} -e s/yy/cc/g -e s/YY/CC/g < y.tab.c > gram.c
	${SED} -e s/yy/cc/g -e s/YY/CC/g < y.tab.h > cc.tab.h
	${RM} y.tab.c y.tab.h
	${MV} y.output cc.output

mgram.c:	mgram.y
	${YACC} ${YFLAGS} $<
	${SED} -e s/yy/mm/g -e s/YY/MM/g < y.tab.c > mgram.c
	${SED} -e s/yy/mm/g -e s/YY/MM/g < y.tab.h > mm.tab.h
	${RM} y.tab.c y.tab.h
	${MV} y.output mm.output

cscan.c:	scan.l
	${LEX} ${LFLAGS} $<
	${SED} -e s/yy/cc/g -e s/YY/CC/g < lex.yy.c > cscan.c
	${RM} lex.yy.c

mscan.c:	scan.l
	${LEX} ${LFLAGS} $<
	${SED} -e s/yy/mm/g -e s/YY/MM/g < lex.yy.c > mscan.c
	${RM} lex.yy.c
	
objdircre:
	@ mkdir -p ${WIND_HOST_TYPE}

clean:
	${RM} -r ./${WIND_HOST_TYPE} $(CLEANFILES)
