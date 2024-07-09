#	@(#)Makefile	8.1 (Berkeley) 5/31/93

SUBDIR=	cat chmod cp csh date dd df echo ed expr grep hostname kill ln ls \
	mkdir mt mv pax ping ps pwd rm rmail rmdir sh sleep stty \
	sync test

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc_v9"
SUBDIR+=ps.v8 ps.v9
.endif

.include <bsd.subdir.mk>
