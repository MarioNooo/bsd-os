#	BSDI Makefile,v 1.2 1998/03/31 18:29:03 donn Exp

SUBDIR=	libmstd

.if exists(${.CURDIR}/libm${MACHINE})
SUBDIR+=libm${MACHINE} mathlink

# Mathlink will set up most of the needed links at boot time.
# We add symlinks here from /usr to /var so that /usr can be read-only.
afterinstall:
	-SLIB=$$( \
	    (echo 'MAP() { [ X$$1 = X-lm ] && echo $$5; }'; \
		cat ${DESTDIR}/etc/shlib.map) | \
	    sh); \
	DLIB=$$(echo $$SLIB | sed -e 's/_s\..*$$/.so/'); \
	ALIB=$$( \
	    (echo 'MAP() { [ X$$1 = X-lm ] && echo $$4; }'; \
		cat ${DESTDIR}/etc/shlib.map) | \
	    sh); \
	PLIB=$$(dirname $$ALIB)/$$(basename $$ALIB .a)_p.a; \
	STUB=$$(dirname $$ALIB)/$$(basename $$SLIB).a; \
	EXCEPT=$$(dirname $$ALIB)/libm.except; \
	for f in $$SLIB $$STUB $$EXCEPT $$DLIB $$ALIB $$PLIB; \
	do \
		[ $$(expr $$f : /usr) -gt 0 ] && \
		    ln -sf /var/run/$$(basename $$f) ${DESTDIR}$$f; \
	done
.endif

.include <bsd.subdir.mk>
