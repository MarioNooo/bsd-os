#	BSDI	Makefile,v 2.15 2003/06/26 20:52:56 polk Exp

SUBDIR+=bin games libdata libexec old sbin share usr.bin usr.sbin

.if ${MACHINE} == "i386"
#SUBDIR+=sco
.endif

.if !defined(__NODIAG) && exists(diag)
SUBDIR+=diag
.endif

.if defined(NOCRYPT)
# This piece is to rebuild some things without encryption, and
# de-install encryption commands/libraries.
# After building and installing the system, to strip backout encryption:
#	% make -DNOCRYPT clean
#	% make -DNOCRYPT
#	% make -DNOCRYPT install
MAKE+=-DNOCRYPT
SUBDIR=bin/rcp usr.bin/rlogin usr.bin/rsh usr.bin/telnet \
	libexec/identd libexec/rlogind libexec/rshd libexec/telnetd

afterinstall:
	-rm -f ${DESTDIR}/usr/lib/libdes.a \
		${DESTDIR}/usr/lib/libtelnet.a \
		${DESTDIR}/usr/bin/bdes \
		${DESTDIR}/usr/contrib/bin/cattach \
		${DESTDIR}/usr/contrib/bin/ccat \
		${DESTDIR}/usr/contrib/bin/cdetach \
		${DESTDIR}/usr/contrib/bin/cfssh \
		${DESTDIR}/usr/contrib/bin/cmkdir \
		${DESTDIR}/usr/contrib/bin/cname \
		${DESTDIR}/usr/contrib/bin/cpasswd \
		${DESTDIR}/usr/contrib/lib/cfsd \
		${DESTDIR}/usr/sbin/add_preshr_key \
		${DESTDIR}/usr/sbin/add_pub_key \
		${DESTDIR}/usr/sbin/dump_key \
		${DESTDIR}/usr/sbin/fingerprint \
		${DESTDIR}/usr/sbin/gen_dss_key \
		${DESTDIR}/usr/sbin/idecrypt \
		${DESTDIR}/usr/sbin/ikmpd \
		${DESTDIR}/usr/sbin/list_keys \
		${DESTDIR}/usr/sbin/show_preshr
.endif

.include <bsd.subdir.mk>
