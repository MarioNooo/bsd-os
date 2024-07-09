PROG=	hroute6d
SRCS=	main.c startup.c timer.c input.c output.c send_admin.c parse.c \
	tree.c ker_rt.c handler.c exit.c

CFLAGS+=	-I${.CURDIR}/../include -DINET6 -I${.OBJDIR}

MAN5=	hroute6d.conf.0
MAN8=	hroute6d.0

# -DNINSTALL       does not affect kernel FW-table

# bsdi does not respect .PATH on $>
keywords.h: keywords
	sed -e '/^#/d' -e '/^$$/d' $> > _keywords.tmp
	tr a-z A-Z < _keywords.tmp | paste _keywords.tmp - | \
	    awk '{ \
		if (NF > 1) \
			printf "#define\tK_%s\t%d\n\t{\"%s\", K_%s, %d},\n", \
			    $$2, NR, $$1, $$2, length($$2) }' \
	    > ${.TARGET}
	rm -f _keywords.tmp

.include <bsd.prog.mk>

parse.o .depend lint tags: keywords.h
# The line below must be after ".include <bsd.prog.mk>".
CLEANFILES+=	keywords.h
