LIB=db
NODYNAMIC=

.PATH:	${.CURDIR}/db ${.CURDIR}/mpool ${.CURDIR}/hash \
	${.CURDIR}/btree ${.CURDIR}/recno
SRCS =	hash.c hash_bigkey.c hash_debug.c hash_func.c hash_log2.c \
	hash_page.c hsearch.c dbm.c
SRCS+=	bt_close.c bt_conv.c bt_debug.c bt_delete.c bt_get.c \
	bt_open.c bt_overflow.c bt_page.c bt_put.c bt_search.c \
	bt_seq.c bt_split.c bt_utils.c
SRCS+=	db.c
SRCS+= 	mpool.c
SRCS+=	rec_close.c rec_delete.c rec_get.c rec_open.c rec_put.c \
	rec_search.c rec_seq.c rec_utils.c

CFLAGS+= -I${.CURDIR} -I${.CURDIR}/include -I${.CURDIR}/mpool \
	-I${.CURDIR}/db -I${.CURDIR}/hash -I${.CURDIR}/btree \
	-I${.CURDIR}/recno

install:

.include <bsd.lib.mk>
