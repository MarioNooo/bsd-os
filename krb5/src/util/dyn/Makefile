LIB=dyn
CLEANFILES+=tags

LDDYNMAJOR=.1
LDDYNMINOR=.0

CFLAGS+= -I${.CURDIR} -I.

CFLAGS+= -DHAVE_TCL_H=1 -DHAVE_MEMMOVE=1 

SRCS=	dyn_create.c dyn_put.c dyn_debug.c dyn_delete.c dyn_size.c \
	dyn_append.c dyn_realloc.c dyn_paranoid.c dyn_insert.c \
        dyn_initzero.c

.include <bsd.lib.mk>
