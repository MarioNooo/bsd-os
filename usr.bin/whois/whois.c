/*
 * Copyright (c) 1980, 1993
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
static char copyright[] =
"@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)whois.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pathnames.h"

#define	NICHOST "whois.internic.net"

main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	FILE *sfi, *sfo;
	int ch;
	struct sockaddr_in sin;
	struct hostent *hp;
	struct servent *sp;
	int s;
	char *host;
	char *cp, *port = NULL;
	FILE *fp;
	char line[1024];
	int nargc;
	char **nargv;
	int i;

	host = NICHOST;

	if ((fp = fopen(_PATH_WHOISCONF, "r")) != NULL) {
		if (fgets(line, sizeof(line), fp) == NULL) 
			err("whois");
		if ((cp = rindex(line, '\n')) != NULL)
			*cp = '\0';
		host = line;
	}

	/* 
	 * an approximation of getopt(), however we pass on 
	 * what we don't recognize so it can be interpreted by whoisd
	 */
	argc--;
	argv++;
	if ((nargv = malloc(argc * sizeof(char *))) == NULL)
		err(1, NULL);
	nargc = 0;
	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 'h') {
			if (argv[i][2] != 0) { 
				host = &argv[i][2];
				continue;
			} else if ((i + 1) < argc) { 
				i++;
				host = argv[i];
				continue;
			} 
		}
		if (argv[i][0] == '-' && argv[i][1] == 'p') {
			if (argv[i][2] != 0) { 
				port = &argv[i][2];
				continue;
			} else if ((i + 1) < argc) { 
				i++;
				port = argv[i];
				continue;
			} 
		}
                      
		nargv[nargc] = argv[i];
		nargc++;
	}

	if (!nargc)
		usage();

	hp = gethostbyname(host);
	if (hp == NULL) {
		(void)fprintf(stderr, "whois: %s: ", host);
		herror((char *)NULL);
		exit(1);
	}
	host = hp->h_name;
	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) {
		perror("whois: socket");
		exit(1);
	}
	bzero((caddr_t)&sin, sizeof (sin));
	sin.sin_family = hp->h_addrtype;
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("whois: bind");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	if (port != NULL) {
		sin.sin_port = htons(atoi(port));
	} else {
		sp = getservbyname("whois", "tcp");
		if (sp == NULL) {
			(void)fprintf(stderr,
			    "whois: whois/tcp: unknown service\n");
			exit(1);
		}
		sin.sin_port = sp->s_port;
	}
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("whois: connect");
		exit(1);
	}
	sfi = fdopen(s, "r");
	sfo = fdopen(s, "w");
	if (sfi == NULL || sfo == NULL) {
		perror("whois: fdopen");
		(void)close(s);
		exit(1);
	}
	while (nargc-- > 1)
		(void)fprintf(sfo, "%s ", *nargv++);
	(void)fprintf(sfo, "%s\r\n", *nargv);
	(void)fflush(sfo);
	while ((ch = getc(sfi)) != EOF)
		putchar(ch);
	exit(0);
}

usage()
{
	(void)fprintf(stderr, "usage: whois [-h host] [-p port] name ...\n");
	exit(1);
}
