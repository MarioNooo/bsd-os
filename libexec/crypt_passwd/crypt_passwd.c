/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI crypt_passwd.c,v 1.2 1997/01/14 20:21:34 bostic Exp
 */

/*
 * crypt_passwd:
 *
 * crypt()'s a local password, enforcing login.class restrictions.
 *
 * Reads `class' and `password' from stdin (one line each).
 * Outputs the crypt'ed password on stdout.
 *
 * NOTE: error output isn't on stderr because it's hard to setup a
 * pipe loop to process that and this program is meant to be run
 * by other programs -- the exit status indicates if the output
 * is error text or not.
 */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <login_cap.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char   *local_passwd __P((char *, char *));

#define CLASS_LEN 1024

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	char class[CLASS_LEN + 1], pw[_PASSWORD_LEN + 1];

	if (fgets(class, CLASS_LEN, stdin) == NULL) {
		printf("Class and password are required on stdin\n");
		exit(1);
	}
	if ((i = strlen(class)) > 0 && class[i-1] == '\n')
		class[i - 1] = '\0';

	if (fgets(pw, _PASSWORD_LEN, stdin) == NULL) {
		printf("Class and password are required on stdin\n");
		exit(1);
	}
	if ((i = strlen(pw)) > 0 && pw[i-1] == '\n')
		pw[i - 1] = '\0';

	printf("%s\n", local_passwd(class, pw));
	exit(0);
}


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
local_passwd(class, pw)
	char *class, *pw;
{
	login_cap_t *lc;
	int minlen, widep;
	char salt[9];

	widep = 0;
	minlen = 6;
	if ((lc = login_getclass(class)) != NULL) {
		widep = login_getcapbool(lc, "widepasswords", widep);
		minlen = login_getcapnum(lc, "minpasswordlen", minlen, minlen);
	}

	if (minlen < 0 || strlen(pw) < minlen) {
		printf("Passwords must be at least %d characters.\n", minlen);
		exit(1);
	}

	/* Grab a random printable character that isn't a colon. */
	srandom((int)time(NULL));
	if (widep) {
		salt[0] = _PASSWORD_EFMT1;
		to64(&salt[1], (long)(29 * 25), 4);
		to64(&salt[5], random(), 4);
	} else
		to64(&salt[0], random(), 2);

	return (crypt(pw, salt));
}
