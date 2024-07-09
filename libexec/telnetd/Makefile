#	BSDI Makefile,v 2.11 2003/08/21 22:29:28 polk Exp
#
#	@(#)Makefile	8.2 (Berkeley) 12/15/93

PROG=	telnetd
CFLAGS+=-DLINEMODE -DUSE_TERMIO -DDIAGNOSTICS
CFLAGS+=-DOLD_ENVIRON -DENV_HACK -DHAS_CGETENT -DSEND_IPADDR
CFLAGS+=-DAUTHENTICATION -DKRB5_KRB4_COMPAT -I${.CURDIR}/../../domestic
.if !defined(NOCRYPT)
CFLAGS+= -DENCRYPTION
.endif
REL!=	uname -r
.ifnmake(depend)
CFLAGS+=-DDEFAULT_IM="\"\r\nBSDI BSD/OS ${REL} (%h) (%t)\r\n\r\r\n\r\""
.endif
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c getent.c krb_configured.c
DPADD=	${LIBUTIL} ${LIBTERM} ${LIBTELNET}
LDADD=	-lutil -ltermcap -ltelnet
DPADD+=${LIBKRB} ${LIBDES}
LDADD+=-lkrb4 -lkrb524 -ldes425 -lkrb5 -lk5crypto -lprofile -lcom_err
MAN8=	telnetd.0

# These are the sources that have encryption stuff in them.
CRYPT_SRC= authenc.c ext.h state.c telnetd.c termstat.c
CRYPT_SRC+= utility.c Makefile
NOCRYPT_DIR=${.CURDIR}/Nocrypt

.PATH:	${.CURDIR}/../../krb5/src/bsdos/rlogin.krb4

.include <bsd.prog.mk>

nocrypt:
#ifdef	ENCRYPTION
	@for i in ${CRYPT_SRC}; do \
	    if [ ! -d ${NOCRYPT_DIR} ]; then \
		echo Creating subdirectory ${NOCRYPT_DIR}; \
		mkdir ${NOCRYPT_DIR}; \
	    fi; \
	    echo ${NOCRYPT_DIR}/$$i; \
	    unifdef -UENCRYPTION ${.CURDIR}/$$i | \
		sed "s/ || defined(ENCRYPTION)//" > ${NOCRYPT_DIR}/$$i; \
	done

placeholder:
#else	/* ENCRYPTION */
	@echo "Encryption code already removed."
#endif	/* ENCRYPTION */
