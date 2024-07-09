/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI auth_subr.c,v 1.1 1997/08/27 17:11:15 prb Exp
 */
#include <ctype.h>
#include <vis.h>

void
encodestring(char *dst, char *src, int cnt)
{
	dst += strvisx(dst, src, cnt, VIS_WHITE | VIS_CSTYLE);
	*dst++ = '\n';
	*dst = '\0';
}

int
decodestring(char *src)
{
	char *dst;
	int cnt;
	int astate = 0;

	dst = src;
	cnt = 0;
	while (*src && *src != '\n') {
		if (unvis(dst, *src++, &astate, 0) == UNVIS_VALID) {
			++dst;
			++cnt;
		}
	}
	return(cnt);
}

int
cleanstring(char *s)
{
	while (*s && isgraph(*s) && *s != '\\')
		++s;
	return(*s == '\0' ? 1 : 0);
}
