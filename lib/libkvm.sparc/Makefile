#	BSDI Makefile,v 1.6 2002/01/18 21:06:33 polk Exp

# sparc libkvm: we simply install symbolic links for the kvm library.
# these links point to -lkvmv8 here.

LIB=	kvm
TARGET?=kvmv8

# For compatible changes, increment minor
# For incompatible changes use increment major
LDDYNMAJOR=.0
LDDYNMINOR=.0

# defeat all the regular make targets in <bsd.lib.mk>
NOARCHIVE=
NOPROFILE=
clean cleandir obj objdir depend:

# install by removing ${LIB} links and making new ones
install:
	@eval $$( \
	    (echo 'MAP() { [ "X$$1" = "X-l${LIB}" ] || return;' \
		'echo "SHARED=$$5;";' \
		'echo "SHAREDDIR=$$(dirname $$5);";' \
		'echo "STUB=$$(dirname $$4)/$$(basename $$5).a;";' \
		'echo "VER=$$VER;";' \
		'exit 0; }'; cat ${SHLIB_MAP}) | sh; \
	    (echo 'MAP() { [ "X$$1" = "X-l${TARGET}" ] || return;' \
		'echo "TARG_SHARED=$$(basename $$5);";' \
		'echo "TARG_STUB=$$(basename $$5).a";' \
		'exit 0; }'; cat ${SHLIB_MAP}) | sh); \
	LINK() { echo ${DESTDIR}$$1 -\> $$2; rm -f $$1; ln -s $$2 $$1; }; \
	LINK ${LIBDIR}/lib${LIB}.a lib${TARGET}.a; \
	LINK ${LIBDIR}/lib${LIB}_p.a lib${TARGET}_p.a; \
	LINK $$STUB lib${TARGET}_s.$$VER.a; \
	LINK $$SHARED lib${TARGET}_s.$$VER; \
	LINK $$SHAREDDIR/lib${LIB}.so${LDDYNSUF} lib${TARGET}.so${LDDYNSUF}

.include <bsd.lib.mk>
