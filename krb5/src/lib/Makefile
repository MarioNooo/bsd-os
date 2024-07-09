# libraries, I see libraries
SUBDIR= crypto krb5 des425 krb5util kdb gssapi rpc kadm5

.if exists(../Makefile.conf)
.include "../Makefile.conf"
.endif

.if defined(KERBEROS4)
SUBDIR+=krb4
.endif

.include <bsd.subdir.mk>
