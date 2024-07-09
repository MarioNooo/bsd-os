
CFLAGS+= -I${.CURDIR} -I${.CURDIR}/../../../sys

.if exists(${.CURDIR}/../lib/obj)
  LDADD+= ${.CURDIR}/../lib/obj/libatm.a
.else
  LDADD+= ${.CURDIR}/../lib/libatm.a
.endif

.include "../Makefile.inc"
