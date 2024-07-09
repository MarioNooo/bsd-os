#	BSDI Makefile,v 1.5 1999/05/07 00:59:37 donn Exp

#
# Fast(er) i387-specific math library.
#

LIB=	m
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../D/src
AINC=	-I${.CURDIR}/../../../lib/libc/i386
NOMAN=	noman

# Don't set the library version here -- modify ../Makefile.version instead.
.include "../Makefile.version"

SRCS=	e_acos.S e_acosf.c e_acosh.c e_acoshf.c e_asin.S e_asinf.c \
	e_atan2.S e_atan2f.c e_atanh.c e_atanhf.c e_cosh.c e_coshf.c \
	e_exp.S e_expf.c e_fmod.S e_fmodf.c e_gamma.c e_gamma_r.c \
	e_gammaf.c e_gammaf_r.c e_hypot.c e_hypotf.c e_j0.c e_j0f.c \
	e_j1.c e_j1f.c e_jn.c e_jnf.c e_lgamma.c e_lgamma_r.c \
	e_lgammaf.c e_lgammaf_r.c e_log.S e_log10.S e_log10f.c \
	e_logf.c e_pow.c e_powf.c e_rem_pio2.c e_rem_pio2f.c \
	e_remainder.S e_remainderf.c e_scalb.S e_scalbf.c e_sinh.c \
	e_sinhf.c e_sqrt.S e_sqrtf.c k_cos.c k_cosf.c k_rem_pio2.c \
	k_rem_pio2f.c k_sin.c k_sinf.c k_tan.c k_tanf.c \
	s_asinh.c s_asinhf.c s_atan.S s_atanf.c s_cbrt.c s_cbrtf.c \
	s_ceil.S s_ceilf.c s_copysign.S s_copysignf.c s_cos.S \
	s_cosf.c s_erf.c s_erff.c s_expm1.c s_expm1f.c s_fabsf.c \
	s_finite.S s_finitef.c s_floor.S s_floorf.c s_frexpf.c \
	s_ilogb.S s_ilogbf.c s_isnanf.c s_ldexpf.c s_lib_version.c \
	s_log1pf.c s_logb.S s_logbf.c s_matherr.c s_modff.c \
	s_nextafter.c s_nextafterf.c s_rint.S s_rintf.c s_scalbn.S \
	s_scalbnf.c s_signgam.c s_significand.S s_significandf.c \
	s_sin.S s_sinf.c s_tan.S s_tanf.c s_tanh.c s_tanhf.c w_cabs.c \
	w_cabsf.c w_drem.c w_dremf.c

# Already in the C library:
#SRCS+=	s_fabs.c s_frexp.c s_isnan.c s_ldexp.c s_modf.c

# Apparently broken
#SRCS+=	s_log1p.S
SRCS+=	s_log1p.c

LDDYNDEP=-lc
CLEANFILES+=libmi386.a libmi386_p.a libmi386_s libmi386_s.a \
	libmi386.so${LDDYNSUF}

#SRCS+=get_hw_float.c

.PATH: ${.CURDIR}/../D/i387 ${.CURDIR}/../D/src

# XXX There are FOUR (4) copies of the libm*.except file and
# XXX all of them must be identical.  It'd be great if we could avoid this...
install:
	ln -f libm.a libmi386.a
	ln -f libm_p.a libmi386_p.a
	ln -f libm_s libmi386_s
	ln -f libm_s.a libmi386_s.a
	ln -f libm.so${LDDYNSUF} libmi386.so${LDDYNSUF}
	cd ${.CURDIR}; \
		( \
			echo 'LIB=mi386'; \
			echo '.include "${.CURDIR}/../Makefile.version"'; \
			echo '.include <bsd.lib.mk>' \
		) | \
			${MAKE} ${MAKEFLAGS} -f - install

.include <bsd.lib.mk>

# Duplicate part of bsd.lib.mk so that we don't have to change
# the names of assembly source files from the FreeBSD distribution.

.SUFFIXES:
.SUFFIXES: .o .po .do .S .c .8 .7 .6 .5 .4 .3 .2 .1 .0

.S.o:
	${CPP} -E ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
	    ${AS} -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

.S.po:
	${CPP} -E -DPROF ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
	    ${AS} -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}

.S.do:
	${CPP} -E -D__PIC__ ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
	    ${AS} -o ${.TARGET}
