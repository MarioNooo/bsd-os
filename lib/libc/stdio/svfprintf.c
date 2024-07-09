/*	BSDI svfprintf.c,v 2.8 2001/03/14 23:28:50 torek Exp	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)svfprintf.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

/*
 * Actual printf innards.
 *
 * This code is large and complicated...
 */

#include <sys/types.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "local.h"
#include "fvwrite.h"

/* Define FLOATING_POINT to get floating point. */
#define	FLOATING_POINT

/*
 * Flush out all the vectors defined by the given uio,
 * then reset it so that it can be reused.
 */
static int
__sprint(fp, uio)
	FILE *fp;
	struct __suio *uio;
{
	int err;

	if (uio->uio_resid == 0) {
		uio->uio_iovcnt = 0;
		return (0);
	}
	err = __sfvwrite(fp, uio);
	uio->uio_resid = 0;
	uio->uio_iovcnt = 0;
	return (err);
}

/*
 * Helper function for `fprintf to unbuffered unix file': creates a
 * temporary buffer.  We only work on write-only files; this avoids
 * worries about ungetc buffers and so forth.
 */
static int
__sbprintf(fp, fmt, ap)
	FILE *fp;
	const char *fmt;
	va_list ap;
{
	int ret;
	FILE fake;
	unsigned char buf[BUFSIZ];

	/* copy the important variables */
	fake._flags = fp->_flags & ~__SNBF;
	fake._file = fp->_file;
	fake._cookie = fp->_cookie;
	fake._write = fp->_write;

	/* set up the buffer */
	fake._bf._base = fake._p = buf;
	fake._bf._size = fake._w = sizeof(buf);
	fake._lbfsize = 0;	/* not actually used, but Just In Case */
	fake._flfp = NULL;	/* dont do any locking on this stream */

	/* do the work, then copy any error status */
	ret = vfprintf(&fake, fmt, ap);
	if (ret >= 0 && fflush(&fake))
		ret = EOF;
	if (fake._flags & __SERR)
		fp->_flags |= __SERR;
	return (ret);
}

/*
 * Macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * Convert an unsigned long to ASCII for printf purposes, returning
 * a pointer to the first character of the string representation.
 * Octal numbers can be forced to have a leading zero; hex numbers
 * use the given digits.
 */
static char *
__ultoa(val, endp, base, octzero, xdigs)
	u_long val;
	char *endp;
	int base, octzero;
	char *xdigs;
{
	char *cp = endp;
	long sval;

	/*
	 * Handle the three cases separately, in the hope of getting
	 * better/faster code.
	 */
	switch (base) {
	case 10:
		if (val < 10) {	/* many numbers are 1 digit */
			*--cp = to_char(val);
			return (cp);
		}
		/*
		 * On many machines, unsigned arithmetic is harder than
		 * signed arithmetic, so we do at most one unsigned mod and
		 * divide; this is sufficient to reduce the range of
		 * the incoming value to where signed arithmetic works.
		 */
		if (val > LONG_MAX) {
			*--cp = to_char(val % 10);
			sval = val / 10;
		} else
			sval = val;
		do {
			*--cp = to_char(sval % 10);
			sval /= 10;
		} while (sval != 0);
		break;

	case 8:
		do {
			*--cp = to_char(val & 7);
			val >>= 3;
		} while (val);
		if (octzero && *cp != '0')
			*--cp = '0';
		break;

	case 16:
		do {
			*--cp = xdigs[val & 15];
			val >>= 4;
		} while (val);
		break;

	default:			/* oops */
		abort();
	}
	return (cp);
}

/* Identical to __ultoa, but for quads. */
static char *
__uqtoa(val, endp, base, octzero, xdigs)
	u_quad_t val;
	char *endp;
	int base, octzero;
	char *xdigs;
{
	char *cp = endp;
	quad_t sval;

	/* quick test for small values; __ultoa is typically much faster */
	/* (perhaps instead we should run until small, then call __ultoa?) */
	if (val <= ULONG_MAX)
		return (__ultoa((u_long)val, endp, base, octzero, xdigs));
	switch (base) {
	case 10:
		if (val < 10) {
			*--cp = to_char(val % 10);
			return (cp);
		}
		if (val > QUAD_MAX) {
			*--cp = to_char(val % 10);
			sval = val / 10;
		} else
			sval = val;
		do {
			*--cp = to_char(sval % 10);
			sval /= 10;
		} while (sval != 0);
		break;

	case 8:
		do {
			*--cp = to_char(val & 7);
			val >>= 3;
		} while (val);
		if (octzero && *cp != '0')
			*--cp = '0';
		break;

	case 16:
		do {
			*--cp = xdigs[val & 15];
			val >>= 4;
		} while (val);
		break;

	default:
		abort();
	}
	return (cp);
}

#ifdef FLOATING_POINT
#include <math.h>
#include "floatio.h"

#define	BUF		(MAXEXP+MAXFRACT+1)	/* + decimal point */
#define	DEFPREC		6

static char *cvt __P((double, int, int, char *, int *, int, int *));
static int exponent __P((char *, int, int));
extern char *__dtoa __P((double, int, int, int *, int *, char **));
extern char *__freedtoa __P((char *));

#else /* no FLOATING_POINT */

#define	BUF		68

#endif /* FLOATING_POINT */


/*
 * Flags used during conversion.
 */
#define	ALT		0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double; unimplemented */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	SHORTINT	0x040		/* short integer */
#define	ZEROPAD		0x080		/* zero (as opposed to blank) pad */
#define FPT		0x100		/* Floating point number */
int
__svfprintf(fp, fmt0, ap)
	FILE *fp;
	const char *fmt0;
	va_list ap;
{
	char *fmt;		/* format string */
	int ch;			/* character from fmt */
	int n;			/* handy integer (short term usage) */
	char *cp;		/* handy char pointer (short term usage) */
	struct __siov *iovp;	/* for PRINT macro */
	int flags;		/* flags as above */
	int ret;		/* return value accumulator */
	int width;		/* width from format (%8d), or 0 */
	int prec;		/* precision from format (%.3d), or -1 */
	char sign;		/* sign prefix (' ', '+', '-', or \0) */
#ifdef FLOATING_POINT
	char softsign;		/* temporary negative sign for floats */
	double _double;		/* double precision arguments %[eEfgG] */
	int decpt;		/* placement of decimal point */
	int expsize;		/* character count for expstr */
	int ndig;		/* actual number of digits returned by cvt */
	char expstr[7];		/* buffer for exponent string */
	char *cvtcp = NULL;	/* Value returned by cvt (for __freedtoa()) */
	int upper;		/* is e, f, or g uppercase? */
#endif
	u_long	ulval;		/* integer arguments %[diouxX] */
	u_quad_t uqval;		/* %q integers */
	int base;		/* base for [diouxX] conversion */
	int dprec;		/* a copy of prec if [diouxX], 0 otherwise */
	int fieldsz;		/* field size expanded by sign, etc */
	int realsz;		/* field size expanded by dprec */
	int size;		/* size of converted field or string */
	char *xdigs;		/* digits for [xX] conversion */
#define NIOV 8
	struct __suio uio;	/* output information: summary */
	struct __siov iov[NIOV];/* ... and individual io vectors */
	char buf[BUF];		/* space for %c, %[diouxX], %[eEfgG] */
	char ox[2];		/* space for 0x hex-prefix */

	/*
	 * Choose PADSIZE to trade efficiency vs. size.  If larger printf
	 * fields occur frequently, increase PADSIZE and make the initialisers
	 * below longer.
	 */
#define	PADSIZE	16		/* pad chunk size */
	static char blanks[PADSIZE] =
	 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
	static char zeros[PADSIZE] =
	 {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};

	/*
	 * BEWARE, these `goto error' on error, and PAD uses `n'.
	 */
#define	PRINT(ptr, len) { \
	iovp->iov_base = (ptr); \
	iovp->iov_len = (len); \
	uio.uio_resid += (len); \
	iovp++; \
	if (++uio.uio_iovcnt >= NIOV) { \
		if (__sprint(fp, &uio)) \
			goto error; \
		iovp = iov; \
	} \
}
#define	PAD(howmany, with) { \
	if ((n = (howmany)) > 0) { \
		while (n > PADSIZE) { \
			PRINT(with, PADSIZE); \
			n -= PADSIZE; \
		} \
		PRINT(with, n); \
	} \
}
#define	FLUSH() { \
	if (uio.uio_resid && __sprint(fp, &uio)) \
		goto error; \
	uio.uio_iovcnt = 0; \
	iovp = iov; \
}

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() \
	(flags&LONGINT ? va_arg(ap, long) : \
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : \
	    (long)va_arg(ap, int))
#define	UARG() \
	(flags&LONGINT ? va_arg(ap, u_long) : \
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    (u_long)va_arg(ap, u_int))

	/* sorry, fprintf(read_only_file, "") returns EOF, not 0 */
	if (cantwrite(fp))
		return (EOF);

	/* optimise fprintf(stderr) (and other unbuffered Unix files) */
	if ((fp->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) &&
	    fp->_file >= 0)
		return (__sbprintf(fp, fmt0, ap));

	fmt = (char *)fmt0;
	uio.uio_iov = iovp = iov;
	uio.uio_resid = 0;
	uio.uio_iovcnt = 0;
	ret = 0;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
			/* void */;
		if ((n = fmt - cp) != 0) {
			PRINT(cp, n);
			ret += n;
		}
		if (ch == '\0')
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			if ((width = va_arg(ap, int)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				n = va_arg(ap, int);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			width = n;
			goto reswitch;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case 'h':
			flags |= SHORTINT;
			goto rflag;
		case 'l':
			if (flags & LONGINT)
				flags |= QUADINT;
			flags |= LONGINT;
			goto rflag;
		case 'q':
			flags |= QUADINT;
			goto rflag;
		case 'c':
			*(cp = buf) = va_arg(ap, int);
			size = 1;
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			if (flags & QUADINT) {
				uqval = va_arg(ap, quad_t);
				if ((quad_t)uqval < 0) {
					uqval = -uqval;
					sign = '-';
				}
			} else {
				ulval = SARG();
				if ((long)ulval < 0) {
					ulval = -ulval;
					sign = '-';
				}
			}
			base = 10;
			goto number;
#ifdef FLOATING_POINT
		case 'e':
		case 'E':
			if (prec == -1)
				prec = DEFPREC;
			if (prec != 0)
				flags |= ALT;
			prec++;		/* convert to "significant digits" */
			goto fp_begin;
		case 'f':
		case 'F':
			if (prec == -1)
				prec = DEFPREC;
			goto fp_begin;
		case 'g':
		case 'G':
			if (prec == -1)
				prec = DEFPREC;
			else if (prec == 0)
				prec = 1;
fp_begin:
			/*
			 * XXX - should put a "long double" in a separate
			 * variable, and have a separate _cvt (or pass the
			 * double/longdbl by address or similar), instead
			 * of just truncating; but this will serve for now.
			 */
			upper = isupper(ch);
			ch = tolower(ch);
			_double = (flags & LONGDBL) ?
			    va_arg(ap, long double) : va_arg(ap, double);
			/* do this before tricky precision changes */
			if (isinf(_double)) {
				if (_double < 0)
					sign = '-';
				if (width < 8) {
					cp = upper ? "INF" : "inf";
					size = 3;
				} else {
					cp = upper ? "INFINITY" : "infinity";
					size = 8;
				}
				break;
			}
			if (isnan(_double)) {
				cp = upper ? "NAN" : "nan";
				size = 3;
				break;
			}
			flags |= FPT;
			cvtcp = cp = cvt(_double, prec, flags, &softsign,
			    &decpt, ch, &ndig);
			/*
			 * See comments at cvt() for how this next bit
			 * works.
			 */
			if (ndig == 0) {	/* __dtoa oddity */
				ndig = 1;
				decpt = 1;
				cp = "0";	/* we free cvtcp later */
			}
			if (ch == 'g') {
				/* exp = decpt - 1: adjust inequalities */
				if (decpt <= -4 || decpt > prec) {
					ch = 'e';
					if ((flags & ALT) == 0)
						prec = ndig;
				} else {
					/* we know decpt <= prec */
					/* ch = 'f'; */
					if ((flags & ALT) == 0) {
						/* P = max(N - decpt, 0) */
						n = ndig - decpt;
						prec = n < 0 ? 0 : n;
					} else
						prec -= decpt;
				}
			}
			if (ch == 'e') {	/* 'e' or 'E' fmt */
				expsize = exponent(expstr, decpt - 1,
				    upper ? 'E' : 'e');
				size = expsize + prec;
				if (ndig > 1 || flags & ALT)
					++size;
			} else {
				if (decpt >= ndig) {	/* eg, 0, 1, 1200 */
					size = decpt;
					if (prec > 0 || flags & ALT)
						size += prec + 1;
				} else if (decpt > 0)	/* eg, 1.23 */
					size = decpt + 1 + prec;
				else			/* eg, 0.0012 */
					size = 2 + prec;
			}

			if (softsign)
				sign = '-';
			break;
#endif /* FLOATING_POINT */
		case 'n':
			if (flags & QUADINT)
				*va_arg(ap, quad_t *) = ret;
			else if (flags & LONGINT)
				*va_arg(ap, long *) = ret;
			else if (flags & SHORTINT)
				*va_arg(ap, short *) = ret;
			else
				*va_arg(ap, int *) = ret;
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			if (flags & QUADINT)
				uqval = va_arg(ap, u_quad_t);
			else
				ulval = UARG();
			base = 8;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			ulval = (u_long)va_arg(ap, void *);
			base = 16;
			xdigs = "0123456789abcdef";
			flags = (flags & ~QUADINT) | HEXPREFIX;
			ch = 'x';
			goto nosign;
		case 's':
			if ((cp = va_arg(ap, char *)) == NULL)
				cp = "(null)";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p = memchr(cp, 0, prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(cp);
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			if (flags & QUADINT)
				uqval = va_arg(ap, u_quad_t);
			else
				ulval = UARG();
			base = 10;
			goto nosign;
		case 'X':
			xdigs = "0123456789ABCDEF";
			goto hex;
		case 'x':
			xdigs = "0123456789abcdef";
hex:			if (flags & QUADINT)
				uqval = va_arg(ap, u_quad_t);
			else
				ulval = UARG();
			base = 16;
			/* leading 0x/X only if non-zero */
			if (flags & ALT &&
			    (flags & QUADINT ? uqval != 0 : ulval != 0))
				flags |= HEXPREFIX;

			/* unsigned conversions */
nosign:			sign = '\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			cp = buf + BUF;
			if (flags & QUADINT) {
				if (uqval != 0 || prec != 0)
					cp = __uqtoa(uqval, cp, base,
					    flags & ALT, xdigs);
			} else {
				if (ulval != 0 || prec != 0)
					cp = __ultoa(ulval, cp, base,
					    flags & ALT, xdigs);
			}
			size = buf + BUF - cp;
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = '\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeros
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeros required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * fieldsz excludes decimal prec; realsz includes it.
		 * Must adjust fieldsz and dprec by sign or 0x prefix
		 * length, too; the output phase treats them as if they
		 * were "digits" in the numeric string.  (Note that for
		 * floating point output, dprec always <= fieldsz, so
		 * adjusting both identically has no effect.)
		 */
		fieldsz = size;
		if (sign)
			dprec++, fieldsz++;
		else if (flags & HEXPREFIX)
			dprec += 2, fieldsz += 2;
		realsz = dprec > fieldsz ? dprec : fieldsz;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST|ZEROPAD)) == 0)
			PAD(width - realsz, blanks);

		/* prefix */
		if (sign) {
			PRINT(&sign, 1);
		} else if (flags & HEXPREFIX) {
			ox[0] = '0';
			ox[1] = ch;
			PRINT(ox, 2);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
			PAD(width - realsz, zeros);

		/* leading zeros from decimal precision */
		PAD(dprec - fieldsz, zeros);

		/* the string or number proper */
#ifdef FLOATING_POINT
		if ((flags & FPT) == 0) {
			PRINT(cp, size);
		} else if (ch == 'e') {
			if (ndig > 1 || flags & ALT) {
				ox[0] = *cp++;
				ox[1] = '.';
				PRINT(ox, 2);
				PRINT(cp, ndig - 1);
				PAD(prec - ndig, zeros);
			} else	/* XeYYY */
				PRINT(cp, 1);
			PRINT(expstr, expsize);
		} else { /* f */
			int extra0s;   /* extra zeros to print, if any */

			if (decpt >= ndig) {	/* eg, 0, 1, 1200 */
				PRINT(cp, ndig);
				PAD(decpt - ndig, zeros);
				if (prec > 0 || flags & ALT)
					PRINT(".", 1);
				extra0s = prec;
			} else if (decpt > 0) {	/* eg, 1.23 */
				PRINT(cp, decpt);
				PRINT(".", 1);
				PRINT(cp + decpt, ndig - decpt);
				extra0s = prec - (ndig - decpt);
			} else {		/* eg, 0.0012 */
				PRINT("0.", 2);
				PAD(-decpt, zeros);
				PRINT(cp, ndig);
				extra0s = prec - (ndig - decpt);
			}
			PAD(extra0s, zeros);
		}
#else
		PRINT(cp, size);
#endif
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST)
			PAD(width - realsz, blanks);

		/* finally, adjust ret */
		ret += width > realsz ? width : realsz;

		FLUSH();	/* copy out the I/O vectors */

#ifdef FLOATING_POINT
		/* this has to be after FLUSH() or we get garbage */
		if (cvtcp != NULL) {
			__freedtoa(cvtcp);
			cvtcp = NULL;
		}
#endif
	}
done:
	FLUSH();
error:
	return (__sferror(fp) ? EOF : ret);
	/* NOTREACHED */
}

#ifdef FLOATING_POINT

/*
 * Call the workhorse (__dtoa) function to format a double precision
 * number.  The __dtoa function promises to give us either the first
 * ndigits significant digits (mode 2, E and G formats), or the first
 * ndigits significant digits past the decimal point (mode 3).  The
 * decimal point character itself is never stored in the buffer.
 * Instead, we get an indication as to where the decimal point should go.
 *
 * Only "significant" digits get formatted.  There are never any leading
 * or trailing zeros.  If you ask for 5 digits and the value is 1.2340,
 * you get only the 4 digits "1234".  (This sometimes leads to weirdness
 * with actually-zero values, where the first and only zero is not really
 * "trailing"; see below.)
 *
 * For instance, suppose we get back the string "1234", and decpt
 * is set to 0.  That means the string represents ".1234".  If decpt
 * is negative, there are some number of implied leading zeros: -1
 * means ".01234", -5 ".000001234", and so on.  If decpt is positive,
 * the implied point comes in the middle of, or after, the digit
 * sequence: 1 means "1.234", 4 means "1234.0", and 7 means "1234000.0".
 *
 * For E-format notation, that means we always print this as "1.234E",
 * then decpt minus 1 (with appropriate + or - sign).  That is, ".1234"
 * (decpt==0) should come out as "1.234E-01", while "123.4" (decpt==3)
 * should print as "1.234E+02".
 *
 * For F-style conversions, we need to print decpt digits, then the ".",
 * then the rest.  If decpt is negative, we also need leading 0s, while
 * if decpt is "really big", we need trailing 0s.
 *
 * Occasionally we get back the no digits at all, when the value
 * rounds to zero.  We want to print 0 as "0" or "0.000000" or "0e+00",
 * which we would do if we had gotten a one-character digit string
 * of "0" and decpt==1.  For whatever reason, __dtoa gives us weird
 * values for decpt, so we will have to adjust it.  (We leave this
 * to the caller, which must __freedtoa the empty digit buffer.)
 *
 * Typical E format is pretty easy: we ask for P digits, and get N <= P
 * digits.  We print the first one, the ".", the remaining N-1, P-N
 * trailing 0s, then "E+00" or whatever.  The total number of characters
 * to be printed is exactly P + 1 (".") + exponent-size (typically 4;
 * sometimes more for really big exponents).  If the "." is to be omitted
 * (%.0e), it shrinks to P + expsize.
 *
 * Typical F format is harder: we ask for P digits past the decimal
 * point.  We get back N and an implied decimal and there is a good chance
 * lots of zeros went missing.  We basically wind up with three cases:
 *
 *	decpt >= N: there is no fractional part to print.  decpt
 *	is the number of digits to print (including 0s that we must
 *	supply) before the ".".  We print all N digits, then decpt-N
 *	zeros, then the ".", then P more 0s.  Total number of characters
 *	printed: N + (decpt-N) + 1 + P = decpt + 1 + P.
 *
 *	0 < decpt < N: We print the first decpt digits, then ".", then
 *	the N-decpt remaining digits.  Then, if P > N-decpt, we add
 *	P - (N-decpt) 0s.  The case P < N-decpt is "impossible", so
 *	total characters printed is: decpt+1+(N-decpt)+(P-(N-decpt))
 *	= decpt + 1 + P.
 *
 *	decpt <= 0 (all remaining cases): there is no integer part to
 *	print.  We print "0.", add -decpt leading 0s, then print all
 *	N digits, followed by P-(N+(-decpt)) trailing 0s.  The total
 *	number of characters printed is: 2+(-decpt)+N+(P-(N-decpt))
 *	= 2 - decpt + N + (P - N + decpt) = 2 + P.
 *
 * Only one case of F suppresses the decimal point: if decpt >= N and
 * the precision is 0 (and the user did not use "%#.0f" to retain the
 * decimal point), the "." gets omitted.  In this case P (the precision)
 * must be 0, so instead of decpt+1+P, we print just decpt characters.
 *
 * The G format sometimes winds up being identical to the E format.
 * The C Standard says:
 *
 *	... style e (or E) will be used only if the exponent resulting
 *	from such a conversion is less than -4 or greater than or equal
 *	to the precision.  Trailing zeros are removed ...
 *
 * This is exactly the same as E, except for removing trailing zeros.
 * But __dtoa already removed the trailing zeros for us -- all we have
 * to do is reduce the precision to exactly N, and our normal E-case
 * code will do the right thing.  (The alternate version, %#g, keeps the
 * trailing 0s, so we should trim P to N only for regular G.)
 *
 * When the G format uses style F, though, the meaning of the precision
 * changes dramatically.  Instead of being the number of digits after
 * the decimal point, it is the number of "significant digits" to print.
 * This, again, is what __dtoa uses internally -- significant digits.
 * If N==P, __dtoa gave us exactly P significant digits, and we should
 * print those with the decimal point in the right place, and not add
 * any trailing zeros.
 *
 * If N < P, however, the formatted number does not have P significant
 * digits, because there were trailing 0s.  Style G normally suppresses
 * these (just as __dtoa did), but only those *after* the ".", and %#g
 * keeps them all.
 *
 * We can re-use the code that handles F format if we alter P carefully.
 * Suppose we want to drop trailing 0s after the decimal point.  We still
 * have the same three cases (decpt >= N; 0 < decpt < N; decpt <= 0).
 * When decpt >= N, there is no fractional part, and we should set P=0.
 * When 0 < decpt < N, the fractional part occupies N-decpt digits, so
 * we can just set P=N-decpt.  Finally, when decpt <= 0, the fractional
 * part is all N digits long plus -decpt leading zeros -- which is again
 * just N-decpt.  In other words, we want to set P to N-decpt, or 0,
 * whichever is bigger.
 *
 * On the other hand, suppose we want to keep the trailing 0s, so that
 * we get exactly P "significant digits" (ie, N digits, then P-N trailing
 * 0s).  Let us call PF the number of "fraction digits" needed to get P
 * digits total.  Take the three cases again: if decpt>=N, there is no
 * fractional part, but there are "decpt" digits before the ".", so we
 * want PF = P - decpt zeros after it, to get P digits total.  (PF cannot
 * be negative.  That would mean we need to use more than P digits just
 * to print the number, and in that case, we will have switched to style
 * E already.)  If 0 < decpt < N, there is a fractional part, and it has
 * N-decpt digits in it already, with the remaining "decpt" digits making
 * up the integer part.  We want to set PF = P - decpt, i.e., reduce our
 * fraction precision by the number of whole-integer digits, so that the
 * total digits produced will again be P.  Finally, if decpt <= 0, all
 * N digits are fractional, and there are -decpt leading zeros too; we
 * must *increase* the fractional precision by -decpt, i.e., set PF =
 * P + -decpt.  But this is just PF = P - decpt again -- so in all cases,
 * we just want P -= decpt.
 *
 * This method treats a value that is exactly 0.0 as if the integer "0"
 * part is its one significant digit: "%#.3g", applied to 0.0, produces
 * 0.00 (3 digits total).  That seems to be what the Standard means when
 * it calls for 3 "significant digits", even though technically *none*
 * of those digits are significant.
 */
static char *
cvt(value, ndigits, flags, sign, decpt, ch, length)
	double value;
	int ndigits, flags, *decpt, ch, *length;
	char *sign;
{
	int mode, dsgn;
	char *digits, *bp, *rve;

	mode = ch == 'f' ? 3 : 2;
	digits = __dtoa(value, mode, ndigits, decpt, &dsgn, &rve);
	*sign = dsgn ? '-' : '\0';
	*length = rve - digits;
	return (digits);
}

static int
exponent(p0, exp, fmtch)
	char *p0;
	int exp, fmtch;
{
	char *p, *t;
	char expbuf[MAXEXP];

	p = p0;
	*p++ = fmtch;
	if (exp < 0) {
		exp = -exp;
		*p++ = '-';
	}
	else
		*p++ = '+';
	t = expbuf + MAXEXP;
	if (exp > 9) {
		do {
			*--t = to_char(exp % 10);
		} while ((exp /= 10) > 9);
		*--t = to_char(exp);
		for (; t < expbuf + MAXEXP; *p++ = *t++);
	}
	else {
		*p++ = '0';
		*p++ = to_char(exp);
	}
	return (p - p0);
}
#endif /* FLOATING_POINT */
