# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
# BSDI Makefile.gnu,v 2.1 2001/11/09 15:20:19 erwan Exp
#

#
# GNU Makefile: used to install mkdep in a cross development environment.
#

CP		=	cp -fp
CHMOD		=	chmod
CHMODFLAGS	=	0555

all:
	$(CP) mkdep.gcc.sh ${WIND_BASE}/host/${WIND_HOST_TYPE}/bin/mkdep
	$(CHMOD) $(CHMODFLAGS) ${WIND_BASE}/host/${WIND_HOST_TYPE}/bin/mkdep

clean:
