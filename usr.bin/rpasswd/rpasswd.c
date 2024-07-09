/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI rpasswd.c,v 1.2 1996/09/15 20:09:51 bostic Exp
 */

/*
 * Copyright (c) 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "extern.h"

static unsigned char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static void
to64(char *s, long v, int n)
{
	while (--n >= 0) {
		*s++ = itoa64[v&0x3f];
		v >>= 6;
	}
}

int
rpasswd(char *user)
{
	uid_t uid;
	int mlen, tries;
	char *p, *password, *t;
	char buf[_PASSWORD_LEN+1], salt[9];

	printf("Changing password for %s.\n", user);

	password = rpasswd_get(user);

	if ((uid = getuid()) && (password == NULL || password[0] == '\0' ||
	    strcmp(crypt(getpass("Old password:"), password), password))) {
		errno = EACCES;
		err(1, NULL);
	}

	mlen = 6;
	for (buf[0] = '\0', tries = 0;;) {
		snprintf(buf, sizeof(buf), "New password: ");
		p = getpass(buf);
		if (!*p)
			errx(0, "password unchanged");
		if (strlen(p) < mlen && (uid != 0 || ++tries < 2)) {
			printf("Please enter a longer password.\n");
			continue;
		}
		for (t = p; *t && islower(*t); ++t);
		if (!*t && (uid != 0 || ++tries < 2)) {
			printf("Please don't use an all-lower case password.\n"
			       "Unusual capitalization, control characters"
			       "or digits are suggested.\n");
			continue;
		}
		strcpy(buf, p);
		if (!strcmp(buf, getpass("Retype new password:")))
			break;
		printf("Mismatch; try again, EOF to quit.\n");
	}

	/* grab a random printable character that isn't a colon */
	srandom((int)time(NULL));
	salt[0] = _PASSWORD_EFMT1;
	to64(&salt[1], (long)(29 * 25), 4);
	to64(&salt[5], random(), 4);

	tries = rpasswd_insert(user, crypt(buf, salt));
	rpasswd_close();
	return (tries);
}
