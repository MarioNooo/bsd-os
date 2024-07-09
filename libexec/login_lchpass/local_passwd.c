/*	BSDI local_passwd.c,v 1.1 1997/08/08 18:58:23 prb Exp	*/

/*-
 * Copyright (c) 1990, 1993, 1994
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

#ifndef lint
static char sccsid[] = "@(#)local_passwd.c	8.3 (Berkeley) 4/2/94";
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <db.h>
#include <fcntl.h>

#include <libpasswd.h>
#include <login_cap.h>

static uid_t uid;

char *tempname;

static unsigned char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void
to64(s, v, n)
	char *s;
	long v;
	int n;
{
	while (--n >= 0) {
		*s++ = itoa64[v&0x3f];
		v >>= 6;
	}
}

char *
getnewpasswd(pw, authenticated)
	struct passwd *pw;
	int authenticated;
{
	int tries;
	char *p, *t;
	char buf[_PASSWORD_LEN+1], salt[9];
	login_cap_t *lc;
	int wp = 0;
	int mlen = 6;
	quad_t expire;

	expire = 0;

	if ((lc = login_getclass(pw->pw_class)) != NULL) {
		wp = login_getcapbool(lc, "widepasswords", wp);
		mlen = login_getcapnum(lc, "minpasswordlen", mlen, mlen);
		pw->pw_change = login_getcaptime(lc, "password-life", 0, 0);
		if (mlen < 0)
			mlen = 0;
		if (mlen > (wp ? _PASSWORD_LEN : 8))
			mlen = wp ? _PASSWORD_LEN : 8;
	} else if (pw->pw_change)
		errx(1, "Could not retrieve password lifetime information.");

	if (pw->pw_change)
		pw->pw_change += (quad_t)time(NULL);

	if (!authenticated) {
		printf("Changing local password for %s.\n", pw->pw_name);

		if (uid && pw->pw_passwd[0] &&
		    strcmp(crypt(getpass("Old password:"), pw->pw_passwd),
		    pw->pw_passwd)) {
			errno = EACCES;
			err(1, NULL);
		}
	}

	for (buf[0] = '\0', tries = 0;;) {
		snprintf(buf, sizeof(buf),
		    "New password (%d significant characters):",
		    wp ? _PASSWORD_LEN : 8);
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
			    "Unusual capitalization, control characters "
			    "or digits are suggested.\n");
			continue;
		}
		strcpy(buf, p);
		if (!strcmp(buf, getpass("Retype new password:")))
			break;
		printf("Mismatch; try again, EOF to quit.\n");
	}
	/* grab a random printable character that isn't a colon */
	srandom((int)time((time_t *)NULL));
	if (wp) {
		salt[0] = _PASSWORD_EFMT1;
		to64(&salt[1], (long)(29 * 25), 4);
		to64(&salt[5], random(), 4);
	} else
		to64(&salt[0], random(), 2);

	return (crypt(buf, salt));
}

int
local_passwd(uname, authenticated)
	char *uname;
	int authenticated;
{
	struct passwd *new, *old;
	char *msg;

	if (!(new = getpwnam(uname)))
		errx(1, "unknown user %s", uname);

	uid = getuid();
	if (uid && uid != new->pw_uid)
		errx(1, "%s", strerror(EACCES));

	endpwent();		/* Close up the database */

	new = pw_copy(new);	/* Make a copy */
	old = pw_copy(new);	/* Make a copy */
	if (new == NULL || old == NULL)
		errx(1, NULL);

	new->pw_passwd = getnewpasswd(new, authenticated);
	msg = pwd_update(old, new, 0);
	if (msg) {
		warnx("%s", msg);
		return (-1);
	}
	return (0);
}
