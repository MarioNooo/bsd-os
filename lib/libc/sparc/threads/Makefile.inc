#	BSDI Makefile.inc,v 1.1 1996/09/25 18:40:31 torek Exp
#
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

CFLAGS+=-I${.CURDIR}/${MACHINE}/threads

SRCS+=	machdep_sync.s thread_context.s thread_stack.c
