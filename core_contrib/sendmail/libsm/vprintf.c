/*
 * Copyright (c) 2000-2001 Sendmail, Inc. and its suppliers.
 *      All rights reserved.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the sendmail distribution.
 */

#include <sm/gen.h>
SM_RCSID("@(#)vprintf.c,v 1.3 2003/09/17 21:19:24 polk Exp")
#include <sm/io.h>
#include "local.h"

/*
**  SM_VPRINTF -- print to standard out with variable length args
**
**	Parameters:
**		timeout -- length of time allow to do the print
**		fmt -- the format of the output
**		ap -- the variable number of args to be used for output
**
**	Returns:
**		as sm_io_vfprintf() does.
*/

int
sm_vprintf(timeout, fmt, ap)
	int timeout;
	char const *fmt;
	SM_VA_LOCAL_DECL
{
	return sm_io_vfprintf(smiostdout, timeout, fmt, ap);
}
