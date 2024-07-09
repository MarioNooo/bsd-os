/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI chap_md5.c,v 1.5 1996/11/22 16:17:45 prb Exp
 */

/*
 * Usage: chap_md5			Produce a challenge
 *	  chap_md5 user id		Produce a response
 *	  chap_md5 -v user id		Verify a response
 *
 * When a response is is being generated, the challenge is passed on stdin.
 * When a challenge is being verified, stdin contains the stread:
 * <number-of-bytes-in-response><response...><challenge...>
 * <number-of-bytes-in-response> is a single byte representing between
 * 0 and 255 bytes in the response.  For MD5 this must be 16.
 */


#include <ctype.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "md5.h"

#define	MAXMSG		256
#define	MAXSECRET	512
#define	MAXNAME		64

#define	_PATH_MD5_SECRETS	"/etc/chap_md5_secrets"

void getsecret(char *, u_char *, int *, int);

void
main(int ac, char **av)
{
	MD5_CTX md5;

	u_char msg[MAXMSG + MAXSECRET];
	u_char resp[16];
	u_char check[16];
	u_char *m;
	u_char *em = msg + sizeof(msg);
	int vflag = 0;
	int c;
	int i;

	while ((c = getopt(ac, av, "v")) != EOF)
		switch(c) {
		case 'v':
			++vflag;
			break;
		default: usage:
			fprintf(stderr, "Usage: chap_md5 [-v] [user id]\n");
			exit(1);
		}

	if (optind == ac) {
		srandom(getpid() ^ time(0));

		i = (random() & 0x1f) + 0x10;

		for (c = 0; c < i; ++c)
			msg[c] = random() & 0xff;
		if (vflag) {
			for (c = 0; c < i; ++c) {
				printf("%02x", msg[c]);
				if ((c & 0x0f) == 0x0f)
					printf("\n");
			}
			printf("\n");
		} else
			write(1, msg, i);
		exit(0);
	}

	if (optind + 2 != ac)
		goto usage;


	/*
	 * read message in
	 */
	m = msg;
	*m++ = atoi(av[optind+1]);

	getsecret(av[optind], m, &i, vflag);

	m += i;

	if (vflag) {
		char ch;

		if (read(0, &ch, 1) != 1)
			errx(1, "missing response");
		if (ch != 0x10)
			errx(1, "invalid response length"); 
		for (i = 0; i < 0x10; i += c)
			if ((c = read(0, &check[i], 0x10 - i)) <= 0)
				errx(1, "short response"); 
	}

	i = 0;

	while (m < em && (c = read(0, m, em - m)) > 0) {
		i += c;
		m += c;
	}
		
	if (i > MAXMSG)
		errx(1, "message too long");

	MD5Init(&md5);
	MD5Update(&md5, msg, m - msg); 
	MD5Final(resp, &md5);

	if (vflag) {
		if (memcmp(resp, check, 0x10))
			errx(1, "invalid response");
		exit(0);
	}
	write(1, resp, 0x10);
	exit(0);
}

void
getsecret(char *user, u_char *secret, int *lenp, int vflag)
{
	FILE *fp;
	char buf[MAXSECRET + MAXNAME + 16];
	int ulen = strlen(user);
	char *p, *q;

	if ((fp = fopen(_PATH_MD5_SECRETS, "r")) == NULL)
		err(1, _PATH_MD5_SECRETS);
	
	while (fgets(buf, sizeof(buf), fp))
		if (buf[ulen] == ':' && strncmp(user, buf, ulen) == 0) {
			p = buf + ulen;
			if (strncasecmp(p, ":challenge:", 11) == 0) {
				if (!vflag)
					continue;
				p += 10;
			}
			if (strncasecmp(p, ":response:", 10) == 0) {
				if (vflag)
					continue;
				p += 9;
			}
			fclose(fp);
			if ((q = strchr(p+1, ':')) == NULL)
				errx(1, "invalid entry in secret database");
			++q;
			if (strncasecmp(p, ":hex:", 5) == 0) {
				*lenp = 0;
				while (isxdigit(q[0]) && isxdigit(q[1])) {
					*secret++ = (digittoint(q[0]) << 4)
					    | digittoint(q[1]);
					q += 2;
					++*lenp;
				}
				if (*q != ':')
					errx(1, "invalid format in secret database (secret)");
			} else if (strncasecmp(p, ":ascii:", 7) == 0) {
				*lenp = 0;
				while (*q && *q != ':') {
					if (q[0] == '\\' && q[1])
						q++;
					*secret++ = *q++;
					++*lenp;
				}
				if (*q != ':')
					errx(1, "invalid format in secret database (secret)");
			} else
				errx(1, "invalid format in secret database (mode)");
			return;
		}

	fclose(fp);
	errx(1, "%s: user unknown", user);
}
