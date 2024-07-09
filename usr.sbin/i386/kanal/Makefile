PROG=kanal
SRCS=
STRIP=
MAN8=	${PROG}.0
DIR_PMAP = ${.CURDIR}/../../../diag/i386/pmap
DIR_IDIFF = ${.CURDIR}/../../../diag/i386/idiff
DIR_FDUMP = ${.CURDIR}/../../../diag/i386/fdump
IPROG	= pmap idiff fdump

kanal:	kanal.perl upax
	cat ${.CURDIR}/kanal.perl upax > kanal
	chmod +x kanal

upax:	$(IPROG)
	pax -w $(IPROG) | gzip | ${.CURDIR}/mdata > upax

pmap:	$(DIR_PMAP)/pmap.c
	(cd $(DIR_PMAP); make)
	if [ -d $(DIR_PMAP)/obj ] ; \
	then	cp $(DIR_PMAP)/obj/pmap . ; \
	else	cp $(DIR_PMAP)/pmap . ; \
	fi

idiff:	$(DIR_IDIFF)/idiff.c
	(cd $(DIR_IDIFF); make)
	if [ -d $(DIR_IDIFF)/obj ] ; \
	then	cp $(DIR_IDIFF)/obj/idiff . ; \
	else	cp $(DIR_IDIFF)/idiff . ; \
	fi

fdump:	$(DIR_FDUMP)/fdump.c
	(cd $(DIR_FDUMP); make)
	if [ -d $(DIR_FDUMP)/obj ] ; \
	then	cp $(DIR_FDUMP)/obj/fdump . ; \
	else	cp $(DIR_FDUMP)/fdump . ; \
	fi

clean:
	rm -f $(IPROG)
	rm -f upax
	rm -f kanal

depend tags:

.include <bsd.prog.mk>
