#	BSDI Makefile.inc,v 1.1 2001/01/30 05:39:14 donn Exp
#
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# add -DKTR and -DDEBUG for thread debugging
CFLAGS+=-I${.CURDIR}/${MACHINE}/threads

SRCS+=	machdep_sync.s thread_context.s thread_stack.c
