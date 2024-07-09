/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ieeefp.h,v 2.2 1998/03/16 18:05:59 donn Exp
 */

#ifndef _IEEEFP_H_
#define	_IEEEFP_H_

/*
 * The interface is defined in fpgetround(3C) on p538 of the SVr4 API,
 * and isnan(3C) on p637.
 */

#include <sys/cdefs.h>

typedef enum {
	FP_SNAN,
	FP_QNAN,
	FP_NINF,
	FP_PINF,
	FP_NDENORM,
	FP_PDENORM,
	FP_NZERO,
	FP_PZERO,
	FP_NNORM,
	FP_PNORM
} fpclass_t;

#include <machine/ieeefp.h>

__BEGIN_DECLS

fp_except fpgetmask __P((void));
fp_rnd fpgetround __P((void));
fp_except fpgetsticky __P((void));
fp_except fpsetmask __P((fp_except));
fp_rnd fpsetround __P((fp_rnd));
fp_except fpsetsticky __P((fp_except));

/* in the math library: */
#define	isnand		isnan
int isnand __P((double));
int isnanf __P((float));
int finite __P((double));

/* currently not in the math library: */
int unordered __P((double, double));
fpclass_t fpclass __P((double));

__END_DECLS

#endif
