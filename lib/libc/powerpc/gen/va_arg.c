/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI va_arg.c,v 1.3 1996/01/30 04:49:42 donn Exp
 */

/*
 * Variadic argument support for C on the Power PC.
 */

#include <sys/param.h>
#include <stdarg.h>

void *
__va_arg(va_list ap, _va_arg_type vat)
{
	void *v;

	switch (vat) {

	case __arg_ARGPOINTER:
		if (ap->__gpr < 8) {
			v = ap->__reg_save_area + ap->__gpr * sizeof (int);
			++ap->__gpr;
		} else {
			v = ap->__overflow_arg_area;
			ap->__overflow_arg_area += sizeof (void *);
		}
		v = *(void **)v;
		break;

	default:
	case __arg_WORD:
		if (ap->__gpr < 8) {
			v = ap->__reg_save_area + ap->__gpr * sizeof (int);
			++ap->__gpr;
		} else {
			v = ap->__overflow_arg_area;
			ap->__overflow_arg_area += sizeof (int);
		}
		break;

	case __arg_DOUBLEWORD:
		if (ap->__gpr < 8 && ap->__gpr & 1)
			++ap->__gpr;
		if (ap->__gpr < 8) {
			v = ap->__reg_save_area + ap->__gpr * sizeof (int);
			ap->__gpr += sizeof (long long) / sizeof (int);
		} else {
			ap->__overflow_arg_area = (char *)
			    roundup((u_int)ap->__overflow_arg_area,
			    sizeof (long long));
			v = ap->__overflow_arg_area;
			ap->__overflow_arg_area += sizeof (long long);
		}
		break;

	case __arg_ARGREAL:
		if (ap->__fpr < 8) {
			v = ap->__reg_save_area + 8 * sizeof (int) +
			    ap->__fpr * sizeof (double);
			++ap->__fpr;
		} else {
			ap->__overflow_arg_area = (char *)
			    roundup((u_int)ap->__overflow_arg_area,
			    sizeof (double));
			v = ap->__overflow_arg_area;
			ap->__overflow_arg_area += sizeof (double);
		}
		break;
	}

	return (v);
}
