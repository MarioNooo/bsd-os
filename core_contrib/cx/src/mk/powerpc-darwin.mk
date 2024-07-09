# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#       BSDI powerpc-darwin.mk,v 2.1 2002/10/28 22:40:57 prb Exp
#

# Build rules for host Darwin

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

LDCC    	=       cc
