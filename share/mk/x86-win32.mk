# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#       BSDI x86-win32.mk,v 2.1 2001/12/21 14:41:13 erwan Exp
#

# Build rules for host WIN32

BINOWN  	=       `id -u`
BINGRP  	=       `id -g`
RADOWN		=	${BINOWN}
RADGRP		=	${BINGRP}
OPEGRP		=	${BINGRP}
NEWOWN		=	${BINOWN}
NEWGRP		=	${BINGRP}
UTMPGRP		=	${BINGRP}
NOOWN		=	${BINOWN}
NOGRP		=	${BINGRP}

SORTOBJS	=	${OBJS}
SORTPOBJS	=	${POBJS}

SUFFIX		=	.exe
ED		=	ex
LEX		=	flex
YACC		=	bison -y

LDCC		=	gcc
MAKE		=	pmake
