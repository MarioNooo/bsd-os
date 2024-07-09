# pick up top-level configuration defines...

.if exists(../Makefile.inc)
.include "../Makefile.inc"
.endif

# defines from autoconfiguration
# util/et
CFLAGS+=	-DNO_YYLINENO=1 -DSYS_ERRLIST_DECLARED=1 \
		-DHAVE_SYS_ERRLIST=1 -DHAVE_STRERROR=1 \
		-DHAVE_STDARG_H=1 -DHAVE_STDLIB_H=1
# util/ss
CFLAGS+=	-DHAVE_STRDUP=1 -DUSE_DIRENT_H=1 \
		-DPOSIX_SIGNALS=1 -DRETSIGTYPE=void


