#	BSDI Makefile,v 1.2 2001/11/28 05:18:48 donn Exp
#	$NetBSD: Makefile,v 1.7 1998/12/15 22:07:11 pk Exp $
#
# ldd(1) is toolchain dependent
#

__bsdi__= 1

.ifndef __bsdi__
.include <bsd.own.mk>			# get OBJECT_FMT definition
.else
OBJECT_FMT= ELF
.endif

.if (${OBJECT_FMT} == "ELF")
SUBDIR+= ldd_elf
.else
SUBDIR+= ldd_aout
.endif

.include <bsd.subdir.mk>
