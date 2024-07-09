/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI htons.c,v 1.4 2001/10/30 22:51:58 polk Exp
 */

#include <sys/param.h>

#undef htons

u_int16m_t
htons(u_int16m_t h)
{

	return (h);
}
