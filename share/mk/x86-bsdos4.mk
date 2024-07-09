# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#       BSDI x86-bsdos4.mk,v 2.2 2002/02/11 20:22:25 prb Exp
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
