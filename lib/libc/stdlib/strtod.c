/*     BSDI strtod.c,v 2.8 2000/02/24 18:14:57 jch Exp */

/****************************************************************
 *
 * The author of this software is David M. Gay.
 *
 * Copyright (c) 1991 by Lucent Technologies.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR LUCENT MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/

/* Please send bug reports to
	David M. Gay
	Bell Laboratories, Room 2C-463
	600 Mountain Avenue
	Murray Hill, NJ 07974-0636
	U.S.A.
	dmg@bell-labs.com
 */

#include "bigint.h"

 static Bigint *
s2b
#ifdef KR_headers
	(s, nd0, nd, y9) CONST char *s; int nd0, nd; ULong y9;
#else
	(CONST char *s, int nd0, int nd, ULong y9)
#endif
{
	Bigint *b;
	int i, k;
	Long x, y;

	x = (nd + 8) / 9;
	for(k = 0, y = 1; x > y; y <<= 1, k++) ;
#ifdef Pack_32
	b = Balloc(k);
	b->x[0] = y9;
	b->wds = 1;
#else
	b = Balloc(k+1);
	b->x[0] = y9 & 0xffff;
	b->wds = (b->x[1] = y9 >> 16) ? 2 : 1;
#endif

	i = 9;
	if (9 < nd0) {
		s += 9;
		do b = multadd(b, 10, *s++ - '0');
			while(++i < nd0);
		s++;
		}
	else
		s += 10;
	for(; i < nd; i++)
		b = multadd(b, 10, *s++ - '0');
	return b;
	}

 static double
ulp
#ifdef KR_headers
	(x) double x;
#else
	(double x)
#endif
{
	register Long L;
	double a;

	L = (word0(x) & Exp_mask) - (P-1)*Exp_msk1;
#ifndef Sudden_Underflow
	if (L > 0) {
#endif
#ifdef IBM
		L |= Exp_msk1 >> 4;
#endif
		word0(a) = L;
		word1(a) = 0;
#ifndef Sudden_Underflow
		}
	else {
		L = -L >> Exp_shift;
		if (L < Exp_shift) {
			word0(a) = 0x80000 >> L;
			word1(a) = 0;
			}
		else {
			word0(a) = 0;
			L -= Exp_shift;
			word1(a) = L >= 31 ? 1 : 1 << 31 - L;
			}
		}
#endif
	return dval(a);
	}

 static double
b2d
#ifdef KR_headers
	(a, e) Bigint *a; int *e;
#else
	(Bigint *a, int *e)
#endif
{
	ULong *xa, *xa0, w, y, z;
	int k;
	double d;
#ifdef VAX
	ULong d0, d1;
#else
#define d0 word0(d)
#define d1 word1(d)
#endif

	xa0 = a->x;
	xa = xa0 + a->wds;
	y = *--xa;
#ifdef DEBUG
	if (!y) Bug("zero y in b2d");
#endif
	k = hi0bits(y);
	*e = 32 - k;
#ifdef Pack_32
	if (k < Ebits) {
		d0 = Exp_1 | y >> Ebits - k;
		w = xa > xa0 ? *--xa : 0;
		d1 = y << (32-Ebits) + k | w >> Ebits - k;
		goto ret_d;
		}
	z = xa > xa0 ? *--xa : 0;
	if (k -= Ebits) {
		d0 = Exp_1 | y << k | z >> 32 - k;
		y = xa > xa0 ? *--xa : 0;
		d1 = z << k | y >> 32 - k;
		}
	else {
		d0 = Exp_1 | y;
		d1 = z;
		}
#else
	if (k < Ebits + 16) {
		z = xa > xa0 ? *--xa : 0;
		d0 = Exp_1 | y << k - Ebits | z >> Ebits + 16 - k;
		w = xa > xa0 ? *--xa : 0;
		y = xa > xa0 ? *--xa : 0;
		d1 = z << k + 16 - Ebits | w << k - Ebits | y >> 16 + Ebits - k;
		goto ret_d;
		}
	z = xa > xa0 ? *--xa : 0;
	w = xa > xa0 ? *--xa : 0;
	k -= Ebits + 16;
	d0 = Exp_1 | y << k + 16 | z << k | w >> 16 - k;
	y = xa > xa0 ? *--xa : 0;
	d1 = w << k + 16 | y << k;
#endif
 ret_d:
#ifdef VAX
	word0(d) = d0 >> 16 | d0 << 16;
	word1(d) = d1 >> 16 | d1 << 16;
#else
#undef d0
#undef d1
#endif
	return dval(d);
	}

 static double
ratio
#ifdef KR_headers
	(a, b) Bigint *a, *b;
#else
	(Bigint *a, Bigint *b)
#endif
{
	double da, db;
	int k, ka, kb;

	dval(da) = b2d(a, &ka);
	dval(db) = b2d(b, &kb);
#ifdef Pack_32
	k = ka - kb + 32*(a->wds - b->wds);
#else
	k = ka - kb + 16*(a->wds - b->wds);
#endif
#ifdef IBM
	if (k > 0) {
		word0(da) += (k >> 2)*Exp_msk1;
		if (k &= 3)
			dval(da) *= 1 << k;
		}
	else {
		k = -k;
		word0(db) += (k >> 2)*Exp_msk1;
		if (k &= 3)
			dval(db) *= 1 << k;
		}
#else
	if (k > 0)
		word0(da) += k*Exp_msk1;
	else {
		k = -k;
		word0(db) += k*Exp_msk1;
		}
#endif
	return dval(da) / dval(db);
	}

#ifdef IEEE_Arith
static CONST double tinytens[] = { 1e-16, 1e-32, 1e-64, 1e-128,
#ifdef Avoid_Underflow
		9007199254740992.e-256
#else
		1e-256
#endif
		};
/* The factor of 2^53 in tinytens[4] helps us avoid setting the underflow */
/* flag unnecessarily.  It leads to a song and dance at the end of strtod. */
#define Scale_Bit 0x10
#else
#ifdef IBM
static CONST double tinytens[] = { 1e-16, 1e-32, 1e-64 };
#else
static CONST double tinytens[] = { 1e-16, 1e-32 };
#endif
#endif

 double
strtod
#ifdef KR_headers
	(s00, se) CONST char *s00; char **se;
#else
	(CONST char *s00, char **se)
#endif
{
#ifdef IEEE_Arith
	int scale;
#endif
	int bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c, dsign,
		 e, e1, esign, i, j, k, nd, nd0, nf, nz, nz0, sign;
	CONST char *s, *s0, *s1;
	double aadj, aadj1, adj, rv, rv0;
	Long L;
	ULong y, z;
	Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;
	sign = nz0 = nz = 0;
	dval(rv) = 0.;
	for(s = s00;;s++) switch(*s) {
		case '-':
			sign = 1;
			/* no break */
		case '+':
			if (*++s)
				goto break2;
			/* no break */
		case 0:
			s = s00;
			goto ret;
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
			continue;
		default:
			goto break2;
		}
 break2:
	if (*s == '0') {
		nz0 = 1;
		while(*++s == '0') ;
		if (!*s)
			goto ret;
		}
	s0 = s;
	y = z = 0;
	for(nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
		if (nd < 9)
			y = 10*y + c - '0';
		else if (nd < 16)
			z = 10*z + c - '0';
	nd0 = nd;
	if (c == '.') {
		c = *++s;
		if (!nd) {
			for(; c == '0'; c = *++s)
				nz++;
			if (c > '0' && c <= '9') {
				s0 = s;
				nf += nz;
				nz = 0;
				goto have_dig;
				}
			goto dig_done;
			}
		for(; c >= '0' && c <= '9'; c = *++s) {
 have_dig:
			nz++;
			if (c -= '0') {
				nf += nz;
				for(i = 1; i < nz; i++)
					if (nd++ < 9)
						y *= 10;
					else if (nd <= DBL_DIG + 1)
						z *= 10;
				if (nd++ < 9)
					y = 10*y + c;
				else if (nd <= DBL_DIG + 1)
					z = 10*z + c;
				nz = 0;
				}
			}
		}
 dig_done:
	e = 0;
	if (c == 'e' || c == 'E') {
		if (!nd && !nz && !nz0) {
			s = s00;
			goto ret;
			}
		s00 = s;
		esign = 0;
		switch(c = *++s) {
			case '-':
				esign = 1;
			case '+':
				c = *++s;
			}
		if (c >= '0' && c <= '9') {
			while(c == '0')
				c = *++s;
			if (c > '0' && c <= '9') {
				L = c - '0';
				s1 = s;
				while((c = *++s) >= '0' && c <= '9')
					L = 10*L + c - '0';
				if (s - s1 > 8 || L > 19999)
					/* Avoid confusion from exponents
					 * so large that e might overflow.
					 */
					e = 19999; /* safe for 16 bit ints */
				else
					e = (int)L;
				if (esign)
					e = -e;
				}
			else
				e = 0;
			}
		else
			s = s00;
		}
	if (!nd) {
		if (!nz && !nz0) {
#ifdef INFNAN_CHECK
			/* Check for Nan and Infinity */
			switch(c) {
			  case 'i':
			  case 'I':
				if (match(&s,"nfinity") || match(&s,"nf")) {
					word0(rv) = 0x7ff00000;
					word1(rv) = 0;
					goto ret;
					}
				break;
			  case 'n':
			  case 'N':
				if (match(&s, "an")) {
					word0(rv) = NAN_WORD0;
					word1(rv) = NAN_WORD1;
					goto ret;
					}
			  }
#endif /* INFNAN_CHECK */
			s = s00;
			}
		goto ret;
		}
	e1 = e -= nf;

	/* Now we have nd0 digits, starting at s0, followed by a
	 * decimal point, followed by nd-nd0 digits.  The number we're
	 * after is the integer represented by those digits times
	 * 10**e */

	if (!nd0)
		nd0 = nd;
	k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
	dval(rv) = y;
	if (k > 9)
		dval(rv) = tens[k - 9] * dval(rv) + z;
	bd0 = 0;
	if (nd <= DBL_DIG
#ifndef RND_PRODQUOT
		&& FLT_ROUNDS == 1
#endif
			) {
		if (!e)
			goto ret;
		if (e > 0) {
			if (e <= Ten_pmax) {
#ifdef VAX
				goto vax_ovfl_check;
#else
				/* rv = */ rounded_product(dval(rv), tens[e]);
				goto ret;
#endif
				}
			i = DBL_DIG - nd;
			if (e <= Ten_pmax + i) {
				/* A fancier test would sometimes let us do
				 * this for larger i values.
				 */
				e -= i;
				dval(rv) *= tens[i];
#ifdef VAX
				/* VAX exponent range is so narrow we must
				 * worry about overflow here...
				 */
 vax_ovfl_check:
				word0(rv) -= P*Exp_msk1;
				/* rv = */ rounded_product(dval(rv), tens[e]);
				if ((word0(rv) & Exp_mask)
				 > Exp_msk1*(DBL_MAX_EXP+Bias-1-P))
					goto ovfl;
				word0(rv) += P*Exp_msk1;
#else
				/* rv = */ rounded_product(dval(rv), tens[e]);
#endif
				goto ret;
				}
			}
#ifndef Inaccurate_Divide
		else if (e >= -Ten_pmax) {
			/* rv = */ rounded_quotient(dval(rv), tens[-e]);
			goto ret;
			}
#endif
		}
	e1 += nd - k;

#ifdef IEEE_Arith
	scale = 0;
#endif

	/* Get starting approximation = rv * 10**e1 */

	if (e1 > 0) {
		if (i = e1 & 15)
			dval(rv) *= tens[i];
		if (e1 &= ~15) {
			if (e1 > DBL_MAX_10_EXP) {
 ovfl:
				errno = ERANGE;
				/* Can't trust HUGE_VAL */
#ifdef IEEE_Arith
				word0(rv) = Exp_mask;
				word1(rv) = 0;
#else
				word0(rv) = Big0;
				word1(rv) = Big1;
#endif
				if (bd0)
					goto retfree;
				goto ret;
				}
			e1 >>= 4;
			for(j = 0; e1 > 1; j++, e1 >>= 1)
				if (e1 & 1)
					dval(rv) *= bigtens[j];
		/* The last multiplication could overflow. */
			word0(rv) -= P*Exp_msk1;
			dval(rv) *= bigtens[j];
			if ((z = word0(rv) & Exp_mask)
			 > Exp_msk1*(DBL_MAX_EXP+Bias-P))
				goto ovfl;
			if (z > Exp_msk1*(DBL_MAX_EXP+Bias-1-P)) {
				/* set to largest number */
				/* (Can't trust DBL_MAX) */
				word0(rv) = Big0;
				word1(rv) = Big1;
				}
			else
				word0(rv) += P*Exp_msk1;
			}
		}
	else if (e1 < 0) {
		e1 = -e1;
		if (i = e1 & 15)
			dval(rv) /= tens[i];
		if (e1 &= ~15) {
			e1 >>= 4;
			if (e1 >= 1 << n_bigtens)
				goto undfl;
#ifdef Avoid_Underflow
			if (e1 & Scale_Bit)
				scale = P;
			for(j = 0; e1 > 0; j++, e1 >>= 1)
				if (e1 & 1)
					dval(rv) *= tinytens[j];
			if (scale && (j = P + 1 - ((word0(rv) & Exp_mask)
						>> Exp_shift)) > 0) {
				/* scaled rv is denormal; zap j low bits */
				if (j >= 32) {
					word1(rv) = 0;
					word0(rv) &= 0xffffffff << j-32;
					if (!word0(rv))
						word0(rv) = 1;
					}
				else
					word1(rv) &= 0xffffffff << j;
				}
#else
			for(j = 0; e1 > 1; j++, e1 >>= 1)
				if (e1 & 1)
					dval(rv) *= tinytens[j];
			/* The last multiplication could underflow. */
			dval(rv0) = dval(rv);
			dval(rv) *= tinytens[j];
			if (!dval(rv)) {
				dval(rv) = 2.*dval(rv0);
				dval(rv) *= tinytens[j];
#endif
				if (!dval(rv)) {
 undfl:
					dval(rv) = 0.;
					errno = ERANGE;
					if (bd0)
						goto retfree;
					goto ret;
					}
#ifndef Avoid_Underflow
				word0(rv) = Tiny0;
				word1(rv) = Tiny1;
				/* The refinement below will clean
				 * this approximation up.
				 */
				}
#endif
			}
		}

	/* Now the hard part -- adjusting rv to the correct value.*/

	/* Put digits into bd: true value = bd * 10^e */

	bd0 = s2b(s0, nd0, nd, y);

	for(;;) {
		bd = Balloc(bd0->k);
		Bcopy(bd, bd0);
		bb = d2b(dval(rv), &bbe, &bbbits);	/* rv = bb * 2^bbe */
		bs = i2b(1);

		if (e >= 0) {
			bb2 = bb5 = 0;
			bd2 = bd5 = e;
			}
		else {
			bb2 = bb5 = -e;
			bd2 = bd5 = 0;
			}
		if (bbe >= 0)
			bb2 += bbe;
		else
			bd2 -= bbe;
		bs2 = bb2;
#ifdef Sudden_Underflow
#ifdef IBM
		j = 1 + 4*P - 3 - bbbits + ((bbe + bbbits - 1) & 3);
#else
		j = P + 1 - bbbits;
#endif
#else
#ifdef Avoid_Underflow
		j = bbe - scale;
#else
		j = bbe;
#endif
		i = j + bbbits - 1;	/* logb(rv) */
		if (i < Emin)	/* denormal */
			j += P - Emin;
		else
			j = P + 1 - bbbits;
#endif
		bb2 += j;
		bd2 += j;
#ifdef Avoid_Underflow
		bd2 += scale;
#endif
		i = bb2 < bd2 ? bb2 : bd2;
		if (i > bs2)
			i = bs2;
		if (i > 0) {
			bb2 -= i;
			bd2 -= i;
			bs2 -= i;
			}
		if (bb5 > 0) {
			bs = pow5mult(bs, bb5);
			bb1 = mult(bs, bb);
			Bfree(bb);
			bb = bb1;
			}
		if (bb2 > 0)
			bb = lshift(bb, bb2);
		if (bd5 > 0)
			bd = pow5mult(bd, bd5);
		if (bd2 > 0)
			bd = lshift(bd, bd2);
		if (bs2 > 0)
			bs = lshift(bs, bs2);
		delta = diff(bb, bd);
		dsign = delta->sign;
		delta->sign = 0;
		i = cmp(delta, bs);
		if (i < 0) {
			/* Error is less than half an ulp -- check for
			 * special case of mantissa a power of two.
			 */
			if (dsign || word1(rv) || word0(rv) & Bndry_mask
#ifdef IEEE_Arith
#ifdef Avoid_Underflow
			 || (word0(rv) & Exp_mask) <= Exp_msk1 + P*Exp_msk1
#else
			 || (word0(rv) & Exp_mask) <= Exp_msk1
#endif
#endif
				) {
#ifdef Avoid_Underflow
				if (!delta->x[0] && delta->wds == 1)
					dsign = 2;
#endif
				break;
				}
			delta = lshift(delta,Log2P);
			if (cmp(delta, bs) > 0)
				goto drop_down;
			break;
			}
		if (i == 0) {
			/* exactly half-way between */
			if (dsign) {
				if ((word0(rv) & Bndry_mask1) == Bndry_mask1
				 &&  word1(rv) == 0xffffffff) {
					/*boundary case -- increment exponent*/
					word0(rv) = (word0(rv) & Exp_mask)
						+ Exp_msk1
#ifdef IBM
						| Exp_msk1 >> 4
#endif
						;
					word1(rv) = 0;
#ifdef Avoid_Underflow
					dsign = 0;
#endif
					break;
					}
				}
			else if (!(word0(rv) & Bndry_mask) && !word1(rv)) {
#ifdef Avoid_Underflow
				dsign = 2;
#endif
 drop_down:
				/* boundary case -- decrement exponent */
#ifdef Sudden_Underflow
				L = word0(rv) & Exp_mask;
#ifdef IBM
				if (L <  Exp_msk1)
#else
				if (L <= Exp_msk1)
#endif
					goto undfl;
				L -= Exp_msk1;
#else
				L = (word0(rv) & Exp_mask) - Exp_msk1;
#endif
				word0(rv) = L | Bndry_mask1;
				word1(rv) = 0xffffffff;
#ifdef IBM
				goto cont;
#else
				break;
#endif
				}
#ifndef ROUND_BIASED
			if (!(word1(rv) & LSB))
				break;
#endif
			if (dsign)
				dval(rv) += ulp(dval(rv));
#ifndef ROUND_BIASED
			else {
				dval(rv) -= ulp(dval(rv));
#ifndef Sudden_Underflow
				if (!dval(rv))
					goto undfl;
#endif
				}
#ifdef Avoid_Underflow
			dsign = 1 - dsign;
#endif
#endif
			break;
			}
		if ((aadj = ratio(delta, bs)) <= 2.) {
			if (dsign)
				aadj = aadj1 = 1.;
			else if (word1(rv) || word0(rv) & Bndry_mask) {
#ifndef Sudden_Underflow
				if (word1(rv) == Tiny1 && !word0(rv))
					goto undfl;
#endif
				aadj = 1.;
				aadj1 = -1.;
				}
			else {
				/* special case -- power of FLT_RADIX to be */
				/* rounded down... */

				if (aadj < 2./FLT_RADIX)
					aadj = 1./FLT_RADIX;
				else
					aadj *= 0.5;
				aadj1 = -aadj;
				}
			}
		else {
			aadj *= 0.5;
			aadj1 = dsign ? aadj : -aadj;
#ifdef Check_FLT_ROUNDS
			switch(FLT_ROUNDS) {
				case 2: /* towards +infinity */
					aadj1 -= 0.5;
					break;
				case 0: /* towards 0 */
				case 3: /* towards -infinity */
					aadj1 += 0.5;
				}
#else
			if (FLT_ROUNDS == 0)
				aadj1 += 0.5;
#endif
			}
		y = word0(rv) & Exp_mask;

		/* Check for overflow */

		if (y == Exp_msk1*(DBL_MAX_EXP+Bias-1)) {
			dval(rv0) = dval(rv);
			word0(rv) -= P*Exp_msk1;
			adj = aadj1 * ulp(dval(rv));
			dval(rv) += adj;
			if ((word0(rv) & Exp_mask) >=
					Exp_msk1*(DBL_MAX_EXP+Bias-P)) {
				if (word0(rv0) == Big0 && word1(rv0) == Big1)
					goto ovfl;
				word0(rv) = Big0;
				word1(rv) = Big1;
				goto cont;
				}
			else
				word0(rv) += P*Exp_msk1;
			}
		else {
#ifdef Sudden_Underflow
			if ((word0(rv) & Exp_mask) <= P*Exp_msk1) {
				dval(rv0) = dval(rv);
				word0(rv) += P*Exp_msk1;
				adj = aadj1 * ulp(dval(rv));
				dval(rv) += adj;
#ifdef IBM
				if ((word0(rv) & Exp_mask) <  P*Exp_msk1)
#else
				if ((word0(rv) & Exp_mask) <= P*Exp_msk1)
#endif
					{
					if (word0(rv0) == Tiny0
					 && word1(rv0) == Tiny1)
						goto undfl;
					word0(rv) = Tiny0;
					word1(rv) = Tiny1;
					goto cont;
					}
				else
					word0(rv) -= P*Exp_msk1;
				}
			else {
				adj = aadj1 * ulp(dval(rv));
				dval(rv) += adj;
				}
#else
			/* Compute adj so that the IEEE rounding rules will
			 * correctly round rv + adj in some half-way cases.
			 * If rv * ulp(rv) is denormalized (i.e.,
			 * y <= (P-1)*Exp_msk1), we must adjust aadj to avoid
			 * trouble from bits lost to denormalization;
			 * example: 1.2e-307 .
			 */
#ifdef Avoid_Underflow
			if (y <= P*Exp_msk1 && aadj > 1.)
#else
			if (y <= (P-1)*Exp_msk1 && aadj > 1.)
#endif
				{
				aadj1 = (double)(int)(aadj + 0.5);
				if (!dsign)
					aadj1 = -aadj1;
				}
#ifdef Avoid_Underflow
			if (scale && y <= P*Exp_msk1)
				word0(aadj1) += (P+1)*Exp_msk1 - y;
#endif
			adj = aadj1 * ulp(dval(rv));
			dval(rv) += adj;
#endif
			}
		z = word0(rv) & Exp_mask;
#ifdef Avoid_Underflow
		if (!scale)
#endif
		if (y == z) {
			/* Can we stop now? */
			L = aadj;
			aadj -= L;
			/* The tolerances below are conservative. */
			if (dsign || word1(rv) || word0(rv) & Bndry_mask) {
				if (aadj < .4999999 || aadj > .5000001)
					break;
				}
			else if (aadj < .4999999/FLT_RADIX)
				break;
			}
 cont:
		Bfree(bb);
		Bfree(bd);
		Bfree(bs);
		Bfree(delta);
		}
#ifdef Avoid_Underflow
	if (scale) {
		word0(rv0) = Exp_1 - P*Exp_msk1;
		word1(rv0) = 0;
		if ((word0(rv) & Exp_mask) <= P*Exp_msk1
		 && word1(rv) & 1
		 && dsign != 2)
			if (dsign) {
#ifdef Sudden_Underflow
				/* rv will be 0, but this would give the  */
				/* right result if only rv *= rv0 worked. */
				word0(rv) += P*Exp_msk1;
				word0(rv0) = Exp_1 - 2*P*Exp_msk1;
#endif
				dval(rv) += ulp(dval(rv));
				}
			else
				word1(dval(rv)) &= ~1;
		dval(rv) *= dval(rv0);
		if (word0(rv) == 0 && word1(rv) == 0)
			errno = ERANGE;
		}
#endif /* Avoid_Underflow */
 retfree:
	Bfree(bb);
	Bfree(bd);
	Bfree(bs);
	Bfree(bd0);
	Bfree(delta);
 ret:
	if (se)
		*se = (char *)s;
	return sign ? -dval(rv) : dval(rv);
	}
