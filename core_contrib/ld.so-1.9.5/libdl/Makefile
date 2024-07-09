#	BSDI Makefile,v 1.10 1999/03/11 19:13:51 jch Exp

.if ${MACHINE} == "sparc_v9"
MCH=	sparc
.else
MCH=	${MACHINE}
.endif

# Deal with 4.0 being released w/o a major version.
# For compatible changes, change this to .0.1, .0.2, ...
# For incompatible changes use LDDYNMAJOR=.1 and reset LDDYNMINOR to .0
LDDYNMAJOR=
LDDYNMINOR=.0.0
LDDYNSUF=${LDDYNMAJOR}${LDDYNMINOR}

PROG=	libdl.so

SRCS=   dlib.c

DLINK=	${.CURDIR}/../D/d-link

CFLAGS+=-fPIC -I- -I${.CURDIR}/../include -I${DLINK}/libdl \
	-I${DLINK}/${MCH} -I${DLINK} \
	-D__PIC__ -DIBCS_COMPATIBLE -DNO_UNDERSCORE -DUSE_CACHE \
	-DUSE_WEAK -DVERBOSE_DLINKER
LDCC=	cc -shared -Wl,-h,${PROG}${LDDYNMAJOR} -o ${PROG}${LDDYNSUF}
STRIP=
MAN3=	dlopen.0
MLINKS=	dlopen.3 dlsym.3 dlopen.3 dlclose.3 dlopen.3 dlerror.3 \
	dlopen.3 dladdr.3
BINDIR=	/shlib

.PATH:	${DLINK}/libdl

.include <bsd.prog.mk>
