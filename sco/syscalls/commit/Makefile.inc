#	BSDI Makefile.inc,v 2.2 1996/08/20 17:39:39 donn Exp

COMMIT_ASM=	commit_accept.o commit_close.o commit_connect.o \
		commit_fcntl.o commit_ioctl.o commit_msgrcv.o \
		commit_msgsnd.o commit_open.o commit_read.o \
		commit_recvfrom.o commit_select.o commit_semop.o \
		commit_wait4.o commit_write.o

COMMIT_IASM+=	commit_sigprocmask.o commit_sigreturn.o commit_sigsuspend.o

OBJS+=	${COMMIT_ASM} ${COMMIT_IASM}
CLEANFILES+=${COMMIT_IASM} ${COMMIT_IASM}

${COMMIT_ASM}: ${.CURDIR}/SYS.h /usr/include/sys/syscall.h
	@echo creating ${.PREFIX}.o
	@printf '#include "SYS.h"\nRSYSCALL(${.PREFIX:S/^commit_//})\n' | \
	    ${CPP} -DCOMMIT ${CFLAGS:M-[ID]*} ${AINC} | ${AS} -o ${.PREFIX}.o
	@${LD} -x -r ${.PREFIX}.o
	@mv a.out ${.PREFIX}.o

${COMMIT_IASM}: ${.CURDIR}/SYS.h /usr/include/sys/syscall.h
	${CPP} -DCOMMIT ${CFLAGS:M-[ID]*} ${AINC} \
	    ${.CURDIR}/i386/sys/${.PREFIX:S/^commit_//}.s | \
	    ${AS} -o ${.PREFIX}.o
	@${LD} -x -r ${.PREFIX}.o
	@mv a.out ${.PREFIX}.o
