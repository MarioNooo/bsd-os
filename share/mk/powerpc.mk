# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#       BSDI powerpc.mk,v 1.6 2003/01/31 21:04:13 prb Exp
#

# PowerPC Cross Compiler Rules

MACHINE         =       powerpc
WIND_REF_DIR    ?=       ${WIND_BASE}/target/powerpc
DESTDIR		=	${WIND_REF_DIR}

AR		=	arppc
AS		=	asppc
C++C		=	g++ppc
CCCMD		=	ccppc
CPPCMD		=	cppppc
FC		=	f77ppc
LDCMD		=	ldppc
NM		=	nmppc
OBJCOPY		=	objcopyppc
OBJDUMP		=	objdumpppc
RANLIB		=	ranlibppc
SIZE		=	sizeppc
STRIP		=	stripppc
