# BSDI Makefile,v 2.9 2002/01/18 21:06:33 polk Exp

LIB=	rpcsvc
INCDIR=	/usr/include/${LIB}
CFLAGS+=-I${.CURDIR}/..
NOSTATIC=nostatic
LDDYNDEP=-lc

# For compatible changes, increment minor
# For incompatible changes use increment major
LDDYNMAJOR=.0
LDDYNMINOR=.0

XFILES=	bootparam_prot.x klm_prot.x mount.x nfs_prot.x nlm_prot.x \
	rex.x rnusers.x rquota.x rstat.x rwall.x sm_inter.x spray.x \
	yppasswd.x
XFILES1= ${.CURDIR}/yp.x
HFILES= ypclnt.h yp_prot.h	
T_HDRS=	${XFILES:S/.x$/.h/g}
T_SRCS=	${XFILES:S/.x$/.c/g}
CLEANFILES+=	${T_SRCS} ${T_HDRS}

HDRS=	${T_HDRS} ${XFILES1}
SRCS=	${T_SRCS}

beforeinstall: ${HDRS} ${XFILES} ${HFILES}
	cd ${.CURDIR}; \
	    for file in ${XFILES} ${HFILES}; do \
	    cmp -s $$file ${DESTDIR}${INCDIR}/$$file >/dev/null 2>&1 || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		$$file ${DESTDIR}${INCDIR}/$$file; \
		done
	for file in ${HDRS}; do \
	    base=`basename $$file`; \
	    cmp -s $$file ${DESTDIR}${INCDIR}/$$base >/dev/null 2>&1 || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		$$file ${DESTDIR}${INCDIR}/$$base; \
		done

${T_SRCS} ${T_HDRS}: ${XFILES}
	for file in ${.ALLSRC}; do \
		pref=`basename $${file} .x` ; \
		rpcgen -C -h ${.CURDIR}/$${pref}.x -o $${pref}.h ; \
		rpcgen -C -c ${.CURDIR}/$${pref}.x -o $${pref}.c ; \
	done

.include <bsd.lib.mk>
