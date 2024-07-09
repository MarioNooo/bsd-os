/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI license.c,v 2.2 1996/09/16 20:56:01 prb Exp
 */

#include <sys/types.h>
#include <sys/license.h>

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "extern.h"

static u_char bits[] =
    "+-/0123456789:=@ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz ";

int
license_decode(src, lp)
	char *src;
	license_t *lp;
{
	int i, j;
	u_char *p, *s, _s[12];

	for (s = (u_char *)src, i = 0; i < 12; ++i, ++s) {
		if (isspace(*s)) {
			--i;
			continue;
		}
		for (j = 0; j < 64 && *s > bits[j]; ++j)
			;
		if (*s != bits[j])
			return (EINVAL);
		_s[i] = j;
	}
	while (isspace(*s))
		++s;
	if (*s != '\0')
		return (EINVAL);

	memset(lp, 0, sizeof(license_t));
	p = lp->data;

	s = _s;
	for (i = 0; i < 9; i+= 3, p += 3, s += 4) {
		p[0] = (s[0] << 2) | ((s[1] >> 4) & 0x3);
		p[1] = (s[1] << 4) | ((s[2] >> 2) & 0xf);
		p[2] = (s[2] << 6) | s[3];
	}
	return (0);
}

char *
license_encode(lp)
	license_t *lp;
{
	static char *s, _s[15];
	u_char *p;
	int i;

	p = lp->data;

	s = _s;
        for (i = 0; i < 15; i += 5, p += 3) {
                s[i]   =  (p[0] >> 2) & 0x3f;
                s[i+1] = ((p[0] << 4) & 0x30) | ((p[1] >> 4) & 0x0f);
                s[i+2] = ((p[1] << 2) & 0x3c) | ((p[2] >> 6) & 0x03);
                s[i+3] =   p[2] & 0x3f;
		s[i+4] = 64;
        }

        for (i = 0; i < 15; ++i)
                _s[i] = bits[s[i]];
	return (_s);
}
