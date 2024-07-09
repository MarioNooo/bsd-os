#	BSDI Makefile,v 1.1 1998/05/06 19:15:29 donn Exp

PROG=	lddstub

# Link against the archive libraries, not the shared libraries.
#LDCC=	cc -v -static -Wl,-dynamic-linker,/shlib/ld-bsdi.so
LDCC=	cc -v -nodefaultlibs -export-dynamic
LDADD=	/usr/lib/libc.a /usr/lib/libgcc.a /shlib/ld-bsdi.so

BINDIR=	/usr/contrib/libexec
NOMAN=	noman
CLEANFILES+=lddstub.c

.if !exists(${.OBJDIR}/lddstub.c)
lddstub.c:
	echo 'int main() { return (0); }' > lddstub.c
.endif

.include <bsd.prog.mk>
