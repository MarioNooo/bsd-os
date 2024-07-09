#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# machine-independent string sources (from ../../string)
SRCS+=	bcmp.c bcopy.c index.c memchr.c memcmp.c memset.c rindex.c strcat.c \
	strcpy.c strcspn.c strncat.c strncmp.c strncpy.c strpbrk.c \
	strsep.c strspn.c strstr.c

# machine-dependent string sources
SRCS+=	bzero.s ffs.s strcmp.s strlen.s
