/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_context.s,v 1.1 1996/09/25 18:40:46 torek Exp
 */

#include "DEFS.h"

#define LOCORE
#include <machine/trap.h>
#undef LOCORE

/*
 * int thread_machdep_savectx(struct _thread_sparc_state *);
 *
 * Stores current context and returns 0.  A later call to restctx below,
 * given the same state, should `return' from this same call, but will
 * return nonzero.  This is much like setjmp/longjmp, but may cross
 * stacks arbitrarily (and also saves the FPU state, if the FPU is in
 * use).
 */
ENTRY(_thread_machdep_savectx)
	t	ST_FLUSHWIN		/* push all registers to stack */
	st	%o7, [%o0 + 0]		/* stash return PC */
	st	%sp, [%o0 + 4]		/* nb: this is caller's stack */
	st	%fp, [%o0 + 8]		/* and frame */
#ifdef notyet
	/* XXX need to prove to myself that this is correct */
	rd	%psr, %o1
	btst	PSR_EF, %o1		/* stash fpu-enable status, and */
	be	1f			/* skip FPU state save if disabled */
	 st	%o1, [%o0 + 12]
#else
	mov	-1, %o1
	st	%o1, [%o0 + 12]		/* for now, always save fpstate */
#endif
	/* save FPU state */
	st	%fsr, [%o0 + 16 + (32*4)]
	std	%f0, [%o0 + 16 + (0*4)]
	std	%f2, [%o0 + 16 + (2*4)]
	std	%f4, [%o0 + 16 + (4*4)]
	std	%f6, [%o0 + 16 + (6*4)]
	std	%f8, [%o0 + 16 + (8*4)]
	std	%f10, [%o0 + 16 + (10*4)]
	std	%f12, [%o0 + 16 + (12*4)]
	std	%f14, [%o0 + 16 + (14*4)]
	std	%f16, [%o0 + 16 + (16*4)]
	std	%f18, [%o0 + 16 + (18*4)]
	std	%f20, [%o0 + 16 + (20*4)]
	std	%f22, [%o0 + 16 + (22*4)]
	std	%f24, [%o0 + 16 + (24*4)]
	std	%f26, [%o0 + 16 + (26*4)]
	std	%f28, [%o0 + 16 + (28*4)]
	std	%f30, [%o0 + 16 + (30*4)]
#ifdef notyet
1:
#endif
	retl
	 clr	%o0

/*
 * int thread_machdep_restctx(struct _thread_sparc_state *, int);
 *
 * Returns to the savectx() call that was made earlier, but returns
 * its second argument (forced nonzero).
 */
ENTRY(_thread_machdep_restctx)
	save	%sp, -64, %sp		/* move down one frame */
	t	ST_FLUSHWIN		/* save old regs before chg. stack */
	ld	[%i0 + 0], %i7		/* fetch new return PC */
	ld	[%i0 + 4], %fp		/* then new stack */
	/* ??? what if frame in saved frame does not match [%i0+8] frame? */
	restore				/* get on that stack; loads i&l */

#ifdef notyet
	ld	[%o0 + 12], %g1
	btst	PSR_EF, %g1		/* skip FPU reload if not saved */
	be	1f
	 nop
#endif

	/* reload fp registers */
	ld	[%o0 + 16 + (32*4)], %fsr
	ldd	[%o0 + 16 + (0*4)], %f0
	ldd	[%o0 + 16 + (2*4)], %f2
	ldd	[%o0 + 16 + (4*4)], %f4
	ldd	[%o0 + 16 + (6*4)], %f6
	ldd	[%o0 + 16 + (8*4)], %f8
	ldd	[%o0 + 16 + (10*4)], %f10
	ldd	[%o0 + 16 + (12*4)], %f12
	ldd	[%o0 + 16 + (14*4)], %f14
	ldd	[%o0 + 16 + (16*4)], %f16
	ldd	[%o0 + 16 + (18*4)], %f18
	ldd	[%o0 + 16 + (20*4)], %f20
	ldd	[%o0 + 16 + (22*4)], %f22
	ldd	[%o0 + 16 + (24*4)], %f24
	ldd	[%o0 + 16 + (26*4)], %f26
	ldd	[%o0 + 16 + (28*4)], %f28
	ldd	[%o0 + 16 + (30*4)], %f30

#ifdef notyet
1:
#endif

	/*
	 * We are about to return to someone who thought he just called
	 * thread_machdep_savectx(), so return 1 to indicate that this
	 * is really a restore.  (Actually, %o1 holds the value we are
	 * "supposed" to return, but it is always 1 for now.)
	 */
	subcc	%o1, 1, %g0		/* carry = (unsigned)%o1 < 1 */
	retl
	 addx	%o1, %g0, %o0		/* return %o1?%o1:1 */
