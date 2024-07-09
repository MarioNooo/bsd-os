#	BSDI	Makefile,v 2.6 2000/03/09 17:58:59 jch Exp
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

MAN7=	ascii.0 environ.0 hier.0 intro.0 \
	mdoc.0 mdoc.samples.0 operator.0 rs-232.0 
MLINKS+=intro.7 miscellaneous.7
MLINKS+=intro.7 airport.7 intro.7 areacode.7 intro.7 birthtoken.7
MLINKS+=intro.7 flowers.7 intro.7 zipcode.7 intro.7 zipcodes.7 

all: ${MAN7}

clean depend lint tags:

cleandir:
	rm -f ${MAN7}

install: maninstall

.include <bsd.prog.mk>
