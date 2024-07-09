# BSDI Makefile.inc,v 1.3 2003/03/06 20:38:14 dab Exp

.PATH:	${.CURDIR}/../../core_contrib/kame/lib/libinet6

MAN3+=	inet6_option_space.0 \
	inet6_rthdr_space.0 inet6_opt_init.0 inet6_rth_space.0

SRCS+=	vars.c
SRCS+=	rthdr.c ip6opt.c
#SRCS+=	rcmd.c
SRCS+=	scope.c
SRCS+=	dstsort.c

MLINKS+= inet6_option_space.3 inet6_option_init.3 \
	inet6_option_space.3 inet6_option_append.3 \
	inet6_option_space.3 inet6_option_alloc.3 \
	inet6_option_space.3 inet6_option_next.3 \
	inet6_option_space.3 inet6_option_find.3 \
	inet6_rthdr_space.3 inet6_rthdr_init.3 \
	inet6_rthdr_space.3 inet6_rthdr_add.3 \
	inet6_rthdr_space.3 inet6_rthdr_lasthop.3 \
	inet6_rthdr_space.3 inet6_rthdr_reverse.3 \
	inet6_rthdr_space.3 inet6_rthdr_segments.3 \
	inet6_rthdr_space.3 inet6_rthdr_getaddr.3 \
	inet6_rthdr_space.3 inet6_rthdr_getflags.3 \
	inet6_opt_init.3 inet6_opt_append.3 \
	inet6_opt_init.3 inet6_opt_finish.3 \
	inet6_opt_init.3 inet6_opt_set_val.3 \
	inet6_opt_init.3 inet6_opt_next.3 \
	inet6_opt_init.3 inet6_opt_find.3 \
	inet6_opt_init.3 inet6_opt_get_val.3 \
	inet6_rth_space.3 inet6_rth_init.3 \
	inet6_rth_space.3 inet6_rth_add.3 \
	inet6_rth_space.3 inet6_rth_reverse.3 \
	inet6_rth_space.3 inet6_rth_segments.3 \
	inet6_rth_space.3 inet6_rth_getaddr.3
