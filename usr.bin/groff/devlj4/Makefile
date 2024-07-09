#	BSDI Makefile,v 1.1 1995/12/05 20:29:32 polk Exp

.include "../Makefile.defs"

DEV=	devlj4
DEVFILES= ALBB ALBR AOB AOI AOR CB CBI CI CLARENDON CORONET CR GB \
	GBI GI GR LGB LGI LGR MARIGOLD OB OBI OI OR S TB TBI TI \
	TR UB UBI UCB UCBI UCI UCR UI UR generate/Makefile \
	generate/special.map generate/text.map
LJ4RES=300

CLEANFILES+=DESC

tags depend: 

all: DESC

DESC: DESC.in

DESC: DESC.in
	@echo making ${.TARGET}
	@echo "res $(LJ4RES)" >${.TARGET}
	@echo "unitwidth `expr 7620000 / $(LJ4RES)`" >>${.TARGET}
	@cat ${.CURDIR}/DESC.in >>${.TARGET}
	@echo "# For A4 paper use: papersize A4" >>${.TARGET}
	@echo "papersize letter" >>${.TARGET}
	@echo print lpr >>${.TARGET}

install:
	for f in ${DEVFILES}; do \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
			${.CURDIR}/$$f ${DESTDIR}${FONTDIR}/${DEV}/$$f; \
	done
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 DESC \
		${DESTDIR}${FONTDIR}/${DEV}/

.include <bsd.prog.mk>
