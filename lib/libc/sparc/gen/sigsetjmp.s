/*
 * Copyright (c) 1994
 *	Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sigsetjmp.s,v 2.2 1998/08/17 05:30:39 torek Exp
 */

#include "DEFS.h"

#define	_JBLEN	9	/* XXX from setjmp.h */

/*
 * int sigsetjmp(sigjmp_buf jmpbuf, int savemask) {
 *	jmpbuf[_JBLEN] = savemask;
 *	return (savemask ? setjmp(jmpbuf) : _setjmp(jmpbuf));
 * }
 *
 * Note, however, that we must call setjmp/_setjmp from the current
 * frame.  Hence, sigsetjmp() MUST NOT be written in C.
 */
ENTRY(sigsetjmp)
	SET_PIC_NOFRAME(%o2, %o3)
	tst	%o1				! savemask
	be	1f
	 st	%o1, [%o0 + (_JBLEN * 4)]	! jmpbuf[_JBLEN] = savemask

#ifdef __PIC__
	sethi	%hi(setjmp), %o2
	b	2f
	 or	%o2, %lo(setjmp), %o2
1:	set	_setjmp, %o2
2:	ld	[%o3 + %o2], %o2
	jmpl	%o2, %g0
	 nop
	/* NOTREACHED */
#else
	sethi	%hi(setjmp), %o2
	jmpl	%o2 + %lo(setjmp), %g0
	 nop
1:	sethi	%hi(_setjmp), %o2
	jmpl	%o2 + %lo(_setjmp), %g0
	 nop
	/* NOTREACHED */
#endif

/*
 * int siglongjmp(sigjmp_buf jmpbuf, int v) {
 *	return (jmpbuf[_JBLEN] ? longjmp(jmpbuf, v) : _longjmp(jmpbuf, v));
 * }
 *
 * This time we could go ahead and call longjmp (which does not return).
 * Annoyingly, that loses %o7, making traceback difficult, but then
 * longjmp tends to do the same... still, for now, we will use the same
 * technique as above.
 */
ENTRY(siglongjmp)
	SET_PIC_NOFRAME(%o2, %o3)
	ld	[%o0 + (_JBLEN * 4)], %o2	! jmpbuf[_JBLEN]
	tst	%o2
	be	1f
	 /*nop*/
#ifdef __PIC__
	sethi	%hi(longjmp), %o2
	b	2f
	 or	%o2, %lo(longjmp), %o2
1:	set	_longjmp, %o2
2:	ld	[%o3 + %o2], %o2
	jmpl	%o2, %g0
	 nop
#else
	sethi	%hi(longjmp), %o2
	jmpl	%o2 + %lo(longjmp), %g0
	 nop
	/* NOTREACHED */
1:
	sethi	%hi(_longjmp), %o2
	jmpl	%o2 + %lo(_longjmp), %g0
	 nop
	/* NOTREACHED */
#endif
