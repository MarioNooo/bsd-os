# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#       BSDI x86-bsdos5.mk,v 1.1 2002/07/22 16:28:29 seebs Exp
#

# Build rules for host BSD/OS

BINOWN  	=       `id -u`
BINGRP  	=       `id -g`
RADOWN		=	${BINOWN}
RADGRP		=	${BINGRP}
OPEGRP		=	${BINGRP}
NEWOWN		=	${BINOWN}
NEWGRP		=	${BINGRP}
UTMPGRP		=	${BINGRP}
NOOWN		=	${BINOWN}
NOGROUP		=	${BINGRP}

SORTOBJS	=	`lorder ${OBJS} | tsort`
SORTPOBJS	=	`lorder ${POBJS} | tsort`

SUFFIX  	=
ED      	=       ed
LEX     	=       lex
YACC    	=       yacc

LDCC    	=       shlicc
