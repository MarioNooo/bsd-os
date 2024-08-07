#	BSDI Makefile,v 1.5 1999/05/07 00:59:38 donn Exp

#
# Under BSD/OS, we build one or two versions of the math library.
# One version (this one) can be configured to conform to multiple standards,
# and does not use hardware transcendental operations, which makes it suitable
# for use on systems with floating point emulation.
# This version also builds and installs the manual pages.
# The other version is limited to IEEE semantics and uses
# machine-specific code (including assembly code).
# Both math libraries are built as 'libm', but the standard version
# is installed as libmstd, while the other is installed as libm${MACHINE}.
# One library is then configured to be the normal libm.
#

#  @(#)Makefile 5.1beta 93/09/24
#  FreeBSD =Id: Makefile,v 1.19 1997/04/15 14:05:28 bde Exp =
#
#  ====================================================
#  Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
#
#  Developed at SunPro, a Sun Microsystems, Inc. business.
#  Permission to use, copy, modify, and distribute this
#  software is freely granted, provided that this notice
#  is preserved.
#  ====================================================
#
#

#
# There are two options in making libm at fdlibm compile time:
# 	_IEEE_LIBM 	--- IEEE libm; smaller, and somewhat faster
#	_MULTI_LIBM	--- Support multi-standard at runtime by
#			    imposing wrapper functions defined in
#			    fdlibm.h:
#				_IEEE_MODE 	-- IEEE
#				_XOPEN_MODE 	-- X/OPEN
#				_POSIX_MODE 	-- POSIX/ANSI
#				_SVID3_MODE 	-- SVID
#
# Here is how to set up CFLAGS to create the desired libm at
# compile time:
#
# 	CFLAGS = -D_IEEE_LIBM		... IEEE libm (recommended)
#	CFLAGS = -D_SVID3_MODE	... Multi-standard supported
#					    libm with SVID as the
#					    default standard
#	CFLAGS = -D_XOPEN_MODE	... Multi-standard supported
#					    libm with XOPEN as the
#					    default standard
#	CFLAGS = -D_POSIX_MODE	... Multi-standard supported
#					    libm with POSIX as the
#					    default standard
#	CFLAGS = 			... Multi-standard supported
#					    libm with IEEE as the
#					    default standard
#

.PATH:	${.CURDIR}/../D/src ${.CURDIR}/../D/man

CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../D/src

# XXX We build this library as 'libm' to force the static shared library
# XXX to be compatible with the library that is currently installed as libm.
LIB=	m
SRCS =	e_acos.c e_acosf.c e_acosh.c e_acoshf.c e_asin.c e_asinf.c \
	e_atan2.c e_atan2f.c e_atanh.c e_atanhf.c e_cosh.c e_coshf.c e_exp.c \
	e_expf.c e_fmod.c e_fmodf.c e_gamma.c e_gamma_r.c e_gammaf.c \
	e_gammaf_r.c e_hypot.c e_hypotf.c e_j0.c e_j0f.c e_j1.c e_j1f.c \
	e_jn.c e_jnf.c e_lgamma.c e_lgamma_r.c e_lgammaf.c e_lgammaf_r.c \
	e_log.c e_log10.c e_log10f.c e_logf.c e_pow.c e_powf.c e_rem_pio2.c \
	e_rem_pio2f.c e_remainder.c e_remainderf.c e_scalb.c e_scalbf.c \
	e_sinh.c e_sinhf.c e_sqrt.c e_sqrtf.c \
	k_cos.c k_cosf.c k_rem_pio2.c k_rem_pio2f.c k_sin.c k_sinf.c \
	k_standard.c k_tan.c k_tanf.c \
	s_asinh.c s_asinhf.c s_atan.c s_atanf.c s_cbrt.c s_cbrtf.c s_ceil.c \
	s_ceilf.c s_copysign.c s_copysignf.c s_cos.c s_cosf.c s_erf.c s_erff.c \
	s_expm1.c s_expm1f.c s_fabsf.c s_finite.c s_finitef.c \
	s_floor.c s_floorf.c s_frexpf.c s_ilogb.c s_ilogbf.c \
	s_isnanf.c s_ldexpf.c s_lib_version.c s_log1p.c \
	s_log1pf.c s_logb.c s_logbf.c s_matherr.c s_modff.c \
	s_nextafter.c s_nextafterf.c s_rint.c s_rintf.c s_scalbn.c s_scalbnf.c \
	s_signgam.c s_significand.c s_significandf.c s_sin.c s_sinf.c s_tan.c \
	s_tanf.c s_tanh.c s_tanhf.c \
	w_acos.c w_acosf.c w_acosh.c w_acoshf.c w_asin.c w_asinf.c w_atan2.c \
	w_atan2f.c w_atanh.c w_atanhf.c w_cabs.c w_cabsf.c w_cosh.c w_coshf.c \
	w_drem.c w_dremf.c w_exp.c w_expf.c w_fmod.c w_fmodf.c w_gamma.c \
	w_gamma_r.c w_gammaf.c w_gammaf_r.c w_hypot.c w_hypotf.c w_j0.c \
	w_j0f.c w_j1.c w_j1f.c w_jn.c w_jnf.c w_lgamma.c w_lgamma_r.c \
	w_lgammaf.c w_lgammaf_r.c w_log.c w_log10.c w_log10f.c w_logf.c \
	w_pow.c w_powf.c w_remainder.c w_remainderf.c w_scalb.c w_scalbf.c \
	w_sinh.c w_sinhf.c w_sqrt.c w_sqrtf.c \
	w_y0.c w_y0f.c w_y1.c w_y1f.c w_yn.c w_ynf.c
LDDYNDEP=-lc
CLEANFILES+=libmstd.a libmstd_p.a libmstd_s libmstd_s.a libmstd.so${LDDYNSUF}

# Don't set the library version here -- modify ../Makefile.version instead.
.include "../Makefile.version"

# BSD/OS doesn't support HW_FLOATINGPT yet
#SRCS+=	get_hw_float.c

# FreeBSD's C library supplies these functions:
#SRCS+=	s_fabs.c s_frexp.c s_isnan.c s_ldexp.c s_modf.c

MAN3+=	acos.0 acosh.0 asin.0 asinh.0 atan.0 atan2.0 atanh.0 ceil.0 \
	cos.0 cosh.0 erf.0 exp.0 fabs.0 floor.0 fmod.0 hypot.0 ieee.0 \
	ieee_test.0 j0.0 lgamma.0 math.0 matherr.0 rint.0 sin.0 sinh.0 \
	sqrt.0 tan.0 tanh.0

MLINKS+=acos.3 acosf.3
MLINKS+=acosh.3 acoshf.3
MLINKS+=asin.3 asinf.3
MLINKS+=asinh.3 asinhf.3
MLINKS+=atan.3 atanf.3
MLINKS+=atanh.3 atanhf.3
MLINKS+=atan2.3 atan2f.3
MLINKS+=ceil.3 ceilf.3
MLINKS+=cos.3 cosf.3
MLINKS+=cosh.3 coshf.3
MLINKS+=erf.3 erfc.3 erf.3 erff.3 erf.3 erfcf.3
MLINKS+=exp.3 expm1.3 exp.3 log.3 exp.3 log10.3 exp.3 log1p.3 exp.3 pow.3 \
	exp.3 expf.3 exp.3 expm1f.3 exp.3 logf.3 exp.3 log2.3 exp.3 log2f.3 \
	exp.3 powf.3 exp.3 log10f.3 exp.3 log1pf.3
MLINKS+=fabs.3 fabsf.3
MLINKS+=floor.3 floorf.3
MLINKS+=fmod.3 fmodf.3
MLINKS+=hypot.3 cabs.3 hypot.3 cabsf.3 hypot.3 hypotf.3
MLINKS+=ieee.3 copysign.3 ieee.3 copysignf.3 ieee.3 finite.3 ieee.3 finitef.3 \
	ieee.3 ilogb.3 ieee.3 ilogbf.3 ieee.3 nextafter.3 ieee.3 nextafterf.3 \
	ieee.3 remainder.3 ieee.3 remainderf.3 ieee.3 scalbn.3 ieee.3 scalbnf.3
MLINKS+=ieee_test.3 logb.3 ieee_test.3 logbf.3
MLINKS+=ieee_test.3 scalb.3 ieee_test.3 scalbf.3
MLINKS+=ieee_test.3 significand.3 ieee_test.3 significandf.3
MLINKS+=j0.3 j1.3 j0.3 jn.3 j0.3 y0.3 j0.3 y1.3 j0.3 y1f.3 j0.3 yn.3
MLINKS+=j0.3 j0f.3 j0.3 j1f.3 j0.3 jnf.3 j0.3 y0f.3 j0.3 ynf.3
MLINKS+=lgamma.3 gamma.3 lgamma.3 lgammaf.3 lgamma.3 gammaf.3 \
	lgamma.3 lgamma_r.3 lgamma.3 lgammaf_r.3 lgamma.3 gamma_r.3 \
	lgamma.3 gammaf_r.3
MLINKS+=rint.3 rintf.3
MLINKS+=sin.3 sinf.3
MLINKS+=sinh.3 sinhf.3
MLINKS+=sqrt.3 cbrt.3 sqrt.3 cbrtf.3 sqrt.3 sqrtf.3
MLINKS+=tan.3 tanf.3
MLINKS+=tanh.3 tanhf.3

.if exists(${.CURDIR}/../libm${MACHINE})
# XXX There are FOUR (4) copies of the libm*.except file and
# XXX all of them must be identical.  It'd be great if we could avoid this...
install: beforeinstall maninstall
	ln -f libm.a libmstd.a
	ln -f libm_p.a libmstd_p.a
	ln -f libm_s libmstd_s
	ln -f libm_s.a libmstd_s.a
	ln -f libm.so${LDDYNSUF} libmstd.so${LDDYNSUF}
	cd ${.CURDIR}; \
		( \
			echo 'LIB=mstd'; \
			echo '.include "${.CURDIR}/../Makefile.version"'; \
			echo '.include <bsd.lib.mk>' \
		) | \
			${MAKE} ${MAKEFLAGS} -f - install
.endif

# XXX we should have only one math.h, and a rule for installing .h's...
beforeinstall:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 \
		${.CURDIR}/../D/src/math.h ${DESTDIR}/usr/include

.include <bsd.lib.mk>
