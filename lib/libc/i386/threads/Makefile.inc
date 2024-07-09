#	BSDI Makefile.inc,v 1.2 1997/07/25 20:24:19 pjd Exp
#
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# add -DKTR and -DDEBUG for thread debugging
CFLAGS+=-I${.CURDIR}/${MACHINE}/threads

SRCS+=	machdep_sync.s thread_context.s thread_stack.c
