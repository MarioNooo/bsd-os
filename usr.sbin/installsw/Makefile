PROG=installsw
SRCS=cur_inter.c cur_setup.c installsw.c map.c util.c
MAN8=installsw.0
CFLAGS+=-Wall
#CFLAGS+=-DDEBUGGING_EZ_INSTALL

DPADD=${LIBCURSES} ${LIBTERMCAP}
LDADD=-lcurses -ltermcap

.include <bsd.prog.mk>
