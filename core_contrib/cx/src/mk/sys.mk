#	BSDI sys.mk,v 2.7 2002/10/28 22:40:57 prb Exp
#
#	@(#)sys.mk	8.2 (Berkeley) 3/21/94

unix		?=	We run UNIX.

# This mismash of C++ suffixes is deplorable; we can only hope one of
# them wins out in the end.
.SUFFIXES: .out .a .ln .o .c .cc .c++ .cxx .C .F .f .e .r .y .l .s .cl .p .h

.LIBS:		.a

AR		?=	ar
ARFLAGS		?=	rl
RANLIB		?=	ranlib

AS		?=	as
AFLAGS		?=

CCCMD		=	cc

.if ${MACHINE} == "sparc"
CFLAGS		?=	-O4
.else
CFLAGS		?=	-O2
.endif

CPPCMD		=	cpp

C++C		?=	g++
C++FLAGS	?=	-O2

FC		?=	f77
FFLAGS		?=	-O
EFLAGS		?=

LEX		?=	lex
LFLAGS		?=

LDCMD		=	ld
LDFLAGS		?=

LINT		?=	lint
LINTFLAGS	?=	-chapbx

MAKE		?=	make

PC		?=	pc
PFLAGS		?=

RC		?=	f77
RFLAGS		?=

SHELL		?=	sh

YACC		?=	yacc
YFLAGS		?=	-d

PAX		?=	pax

NM		?=	nm

OBJCOPY		?=	objcopy

OBJCOPY		?=	objdump

RANLIB		?=	ranlib

SIZE		?=	size

#
# Automatically use the cross development environment if we are running
# on a supported host with ARCH pointing to a supported target
# A support host must specify WIND_HOST_TYPE and may optionally
# specify WIND_HOST_PATH for additional directory to put in PATH
#
.if defined(ARCH) && defined(MACHINE) && defined(OSNAME)
.if ${OSNAME} == "Darwin"
BSD_CXDIR?=	/usr/local/cx
.else
BSD_CXDIR?=	/usr/cx
.endif

#
# For BSD/OS we only support cross dev from i386 to powerpc at this time
#
.if ${ARCH} == "powerpc" && ${MACHINE} == "i386" && ${OSNAME} == "BSD/OS" && exists(${BSD_CXDIR})
WIND_HOST_TYPE=	x86-bsdos4
.endif

#
# Darwin for PowerPC can only be used in a cross dev mode
#
.if ${MACHINE} == "powerpc" && ${OSNAME} == "Darwin" && exists(${BSD_CXDIR})
WIND_HOST_TYPE=	powerpc-darwin
WIND_HOST_PATH=	${WIND_BASE}/bin:
.endif

#
# If we have identifed a supported host platform, do the shared work
#
.ifdef	WIND_HOST_TYPE
WIND_BASE?=	${BSD_CXDIR}
TORNADO_XDEV?=	1
WIND_REF_DIR?=	${WIND_BASE}/target/${ARCH}
PATH:=		${WIND_HOST_PATH}${WIND_BASE}/host/${WIND_HOST_TYPE}/bin:${PATH}
.EXPORT:        WIND_BASE WIND_HOST_TYPE WIND_REF_DIR PATH
.endif

.endif

#
# Override previous values with host and target specific information
# This must be done here for hosts that we do not auto-detect above
#
.ifdef WIND_HOST_TYPE
.include <${WIND_HOST_TYPE}.mk>
.if defined(ARCH) && defined(TORNADO_XDEV)
.include <${ARCH}.mk>
.endif
.endif

CC		?=	${CCCMD}
CPP		?=	${CPPCMD}
LD		?=	${LDCMD}

.c:
	${CC} ${CFLAGS} ${.IMPSRC} -o ${.TARGET}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

.cc.o .c++.o .cxx.o .C.o:
	${C++C} ${C++FLAGS} -c ${.IMPSRC}

.p.o:
	${PC} ${PFLAGS} -c ${.IMPSRC}

.e.o .r.o .F.o .f.o:
	${FC} ${RFLAGS} ${EFLAGS} ${FFLAGS} -c ${.IMPSRC}

.s.o:
	${AS} ${AFLAGS} -o ${.TARGET} ${.IMPSRC}

.y.o:
	${YACC} ${YFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} -c y.tab.c -o ${.TARGET}
	rm -f y.tab.c

.l.o:
	${LEX} ${LFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} -c lex.yy.c -o ${.TARGET}
	rm -f lex.yy.c

.y.c:
	${YACC} ${YFLAGS} ${.IMPSRC}
	mv y.tab.c ${.TARGET}

.y.cc:
	${YACC} ${YFLAGS} ${.IMPSRC}
	mv y.tab.c ${.PREFIX}.cc
	-[ -f y.tab.h ] && mv y.tab.h ${.PREFIX}.tab.h
.l.c:
	${LEX} ${LFLAGS} ${.IMPSRC}
	mv lex.yy.c ${.TARGET}

.s.out .c.out .o.out:
	${CC} ${CFLAGS} ${.IMPSRC} ${LDLIBS} -o ${.TARGET}

.cc.out .c++.out .cxx.out .C.out:
	${C++C} ${C++FLAGS} ${.IMPSRC} ${LDLIBS} -o ${.TARGET}

.f.out .F.out .r.out .e.out:
	${FC} ${EFLAGS} ${RFLAGS} ${FFLAGS} ${.IMPSRC} \
	    ${LDLIBS} -o ${.TARGET}
	rm -f ${.PREFIX}.o

.y.out:
	${YACC} ${YFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} y.tab.c ${LDLIBS} -ly -o ${.TARGET}
	rm -f y.tab.c

.l.out:
	${LEX} ${LFLAGS} ${.IMPSRC}
	${CC} ${CFLAGS} lex.yy.c ${LDLIBS} -ll -o ${.TARGET}
	rm -f lex.yy.c
