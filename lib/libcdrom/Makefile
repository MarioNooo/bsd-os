#	BSDI Makefile,v 2.8 2002/01/18 21:06:32 polk Exp

LIB=	cdrom
SRCS=	common.c misc.c scsi2.c toshiba.c panasonic.c

# For compatible changes, increment minor
# For incompatible changes use increment major
LDDYNMAJOR=.0
LDDYNMINOR=.0

NOSTATIC=nostatic
LDDYNDEP=-lc
MAN3=	cdrom.0
MLINKS+=cdrom.3 cdopen.3 cdrom.3 cdclose.3 cdrom.3 cdplay.3 cdrom.3 cdstop.3 \
	cdrom.3 cdstatus.3 cdrom.3 cdeject.3 cdrom.3 cdvolume.3 \
	cdrom.3 frame_to_msf.3 cdrom.3 msf_to_frame.3 cdrom.3 cdload.3

CFLAGS+=	-I/sys

.include <bsd.lib.mk>
