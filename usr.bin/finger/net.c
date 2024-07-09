/*	BSDI net.c,v 2.8 1997/09/29 14:04:29 dab Exp	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Tony Nardo of the Johns Hopkins University/Applied Physics Lab.
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
static char sccsid[] = "@(#)net.c	8.3 (Berkeley) 1/2/94";
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <db.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <utmp.h>

#include "finger.h"

void
netfinger(name)
	char *name;
{
	extern int lflag;
	register FILE *fp;
	register int c, lastc;
	struct addrinfo *ai, *ai2, hints;
	char namebuf[NI_MAXHOST];
	int s;
	char *host;

	/* Split into user/host names. */
	if ((host = rindex(name, '@')) == NULL)
		return;
	*host++ = '\0';

	/* get the host and service */

	bzero(&hints, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;
	if ((s = getaddrinfo(host, "finger", &hints, &ai)) != NULL) {
		(void)fprintf(stderr, "finger: getaddrinfo(): %s\n",
		    (s == EAI_SYSTEM) ? strerror(errno) : gai_strerror(s));
		return;
	}

	/* Have valid hostname; identify the host connected with. */
	(void)printf("[%s]\n", ai->ai_canonname);
	for (ai2 = ai; ; ai = ai->ai_next) {
		if ((s = socket(ai->ai_family, ai->ai_socktype,
		    ai->ai_protocol)) < 0) {
			if (ai->ai_next == NULL) {
				perror("finger: socket");
				freeaddrinfo(ai2);
				return;
			}
			continue;
		}
		if (connect(s, ai->ai_addr, ai->ai_addrlen) >= 0)
			break;
		(void)close(s);
		if (ai->ai_next == NULL) {
			if (getnameinfo(ai->ai_addr, ai->ai_addrlen, namebuf,
			    sizeof(namebuf), NULL, 0, NI_NUMERICHOST))
				strcpy(namebuf, ai2->ai_canonname);
			(void)fprintf(stderr, "finger: connect %s: %s\n",
			    namebuf, strerror(errno));
			freeaddrinfo(ai2);
			return;
		}
	}
	freeaddrinfo(ai2);

	/* -l flag for remote fingerd  */
	if (lflag)
		write(s, "/W ", 3);
	/* send the name followed by <CR><LF> */
	(void)write(s, name, strlen(name));
	(void)write(s, "\r\n", 2);

	if ((fp = fdopen(s, "r")) == NULL) {
		(void)fprintf(stderr, "fdopen: %s\n", strerror(errno));
		return;
	}

	/*
	 * Read from the remote system; once we're connected, we assume some
	 * data.  If none arrives, we hang until the user interrupts.
	 *
	 * If we see a <CR> or a <CR> with the high bit set, treat it as
	 * a newline; if followed by a newline character, only output one
	 * newline.
	 *
	 * Otherwise, all high bits are stripped; if it isn't printable and
	 * it isn't a space, we can simply set the 7th bit.  Every ASCII
	 * character with bit 7 set is printable.
	 */ 
	lastc = 0;
	while ((c = getc(fp)) != EOF) {
		c &= 0x7f;
		if (c == 0x0d) {
			if (lastc == '\r')		/* ^M^M - skip dupes */
				continue;
			c = '\n';
			lastc = '\r';
		} else {
			if (!isprint(c) && !isspace(c))
				c |= 0x40;
			if (lastc != '\r' || c != '\n')
				lastc = c;
			else {
				lastc = '\n';
				continue;
			}
		}
		putchar(c);
	}
	if (lastc != '\n')
		putchar('\n');
	(void)fclose(fp);
}
