#	BSDI Makefile,v 2.3 1997/10/31 03:14:06 donn Exp
#	@(#)Makefile	5.2 (Berkeley) 3/5/91
#
# Makefile for BSD-specific syscalls, used in the iBCS2 emulator.
# Based on the BSD libc makefile and sources.

.MAIN: all

AINC=	-I${.CURDIR} -I${.CURDIR}/${MACHINE}
NOPROFILE=noprofile
NOMAN=noman

.include "${.CURDIR}/sys/Makefile.inc"
.include "${.CURDIR}/commit/Makefile.inc"

MAN2=
MLINKS=
OBJS+=	Ovfork.o fork.o lseek.o pipe.o ptrace.o setlogin.o sigpending.o \
	sigprocmask.o sigreturn.o sigsuspend.o syscall.o

${OBJS}: SYS.h

clean:
	rm -f syscalls.o
	rm -f a.out Errs errs mklog core
	rm -f ${OBJS} 
	rm -f lib${LIB}.a

cleandir: clean
	rm -f .depend ${MANALL}

all: ${OBJS}
	${LD} ${LDFLAGS} -r -o syscalls.o ${OBJS}

install depend:

.include <bsd.prog.mk>

.s.o:
	${CPP} -E ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
	    ${AS} -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}
