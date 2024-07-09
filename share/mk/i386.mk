# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
# 
#	BSDI i386.mk,v 2.2 2002/03/18 18:05:31 erwan Exp
# 

# i386 Cross Compiler Rules

MACHINE		=	i386
WIND_REF_DIR	?=	${WIND_BASE}/target/i386
DESTDIR		=	${WIND_REF_DIR}

AR		=	ar386
AS		=	as386
C++C		=	g++386
CCCMD		=	cc386
CPPCMD		=	cpp386
FC		=	f77386
LDCMD		=	ld386
NM		=	nm386
OBJCOPY		=	objcopy386
OBJDUMP		=	objdump386
RANLIB		=	ranlib386
SIZE		=	size386
STRIP		=	strip386
