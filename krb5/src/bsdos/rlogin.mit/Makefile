BINDIR=	/usr/bin
BINOWN=	root
BINMODE=4555

PROG=	rlogin

SRCS=	krlogin.c kcmd.c forward.c

.PATH:	${.CURDIR}/../../appl/bsd

CFLAGS+=-I. -I${DESTDIR}/usr/include/krb5
CFLAGS+=-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/../../include/krb5 \
	-I${.CURDIR}/../../appl/bsd \
	-I${.CURDIR}/../../util/et \
	-I${.CURDIR}/../../lib \
	-I${.CURDIR}/../../util -I${.CURDIR}/../../util/ss/obj \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj

CFLAGS+=-DHAVE_LIBUTIL=1 -DSTDC_HEADERS=1 -DHAVE_ISATTY=1 \
	-DHAVE_INET_ATON=1 -DHAVE_KILLPG=1 -DHAVE_INITGROUPS=1 \
	-DHAVE_SETPRIORITY=1 -DHAVE_SETREUID=1 -DHAVE_WAITPID=1 \
	-DHAVE_SETSID=1 -DHAVE_SETLOGIN=1 -DHAVE_TCGETPGRP=1 \
	-DHAVE_TCSETPGRP=1 -DHAVE_SETPGID=1 -DHAVE_UTIMES=1 \
	-DHAVE_UNISTD_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 \
	-DHAVE_SYS_FILIO_H=1 -DHAVE_SYS_SOCKIO_H=1 -DHAVE_SYS_TTY_H=1 \
	-DHAVE_TTYENT_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_UTMP_H=1 \
	-DHAVE_SYS_TIME_H=1 -DHAVE_SYS_IOCTL_COMPAT_H=1 -DHAVE_PATHS_H=1 \
	-DHAVE_ARPA_NAMESER_H=1 -DHAVE_STDARG_H=1 -DHAVE_GETDTABLESIZE=1 \
	-Dkrb5_sigtype=void -DPOSIX_SIGTYPE=1 -DPOSIX_SIGNALS=1 \
	-DPOSIX_SETJMP=1 -DUSE_DIRENT_H=1 -DPOSIX_TERMIOS=1 \
	-DNO_UT_PID=1 -DNO_UT_TYPE=1 -DNO_UT_EXIT=1 -DHAVE_SETOWN=1 \
	-DSETPGRP_TWOARG=1 -DKRB5_PROVIDE_PROTOTYPES=1 \
	-DHAVE_KRB_GET_ERR_TEXT=1 -DHAVE_KRB_SAVE_CREDENTIALS=1
CFLAGS+=-DKRB5_PATH_RLOGIN=\"/usr/bin/rlogin\"
CFLAGS+=-DUCB_RLOGIN=\"/usr/bin/rlogin\"
CFLAGS+=-DUCB_RSH=\"/usr/bin/rsh\"
CFLAGS+=-DUCB_RCP=\"/bin/rcp\"
CFLAGS+=-DLOGIN_PROGRAM=\"/usr/bin/login\"

LDADD+=	-lkrb4 -ldes425 -lkrb5 -lk5crypto -lss -lprofile -lcom_err

.depend: rlogin.1
CLEANFILES+= rlogin.1

rlogin.1: ${.CURDIR}/../../appl/bsd/rlogin.M
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>
