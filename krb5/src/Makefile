# drive the top-down build of the krb5 source tree
.include "Makefile.conf"

SUBDIR = util include lib

.if defined(KRB524)
SUBDIR+= krb524
.endif

SUBDIR+= kdc

# pmake style directories to build things in...
SUBDIR+= bsdos

.include <bsd.subdir.mk>
