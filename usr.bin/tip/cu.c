/*	BSDI cu.c,v 2.2 1996/07/31 23:29:12 prb Exp	*/

/*
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
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
static char sccsid[] = "@(#)cu.c	5.9 (Berkeley) 6/1/90";
#endif /* not lint */

#include "tip.h"

void	cleanup();

/*
 * Botch the interface to look like cu's
 */
cumain(argc, argv)
	char *argv[];
{
	register int i;
	static char sbuf[12];

	if (argc < 2) {
		printf("usage: cu telno [-t] [-s speed] [-l line] [-#]\n");
		exit(8);
	}
	DV = NOSTR;
	BR = DEFBR;
	for (; argc > 1; argv++, argc--) {
		if (argv[1][0] != '-')
			PN = argv[1];
		else switch (argv[1][1]) {

		case 't':
			DU = 0;
			--argc;
			continue;

		case 'a':
			printf("the -a option has been depricated\n");
			++argv; --argc;
			break;

		case 's':
			if (argc < 3 || atoi(argv[2]) == 0) {
				fprintf(stderr, "cu: unsupported speed %s\n",
					argv[2]);
				exit(3);
			}
			BR = atoi(argv[2]); ++argv; --argc;
			break;

		case 'l':
			DV = argv[2]; ++argv; --argc;
			break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (DV)
				DV[strlen(DV)-1] = argv[1][1];
			break;

		default:
			printf("Bad flag %s", argv[1]);
			break;
		}
	}
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGTERM, cleanup);

	/*
	 * The "cu" host name is used to define the
	 * attributes of the generic dialer.
	 */
	(void)sprintf(sbuf, "cu%d", BR);
	getremote(sbuf);

	if (i == -1) {
		printf("link down\n");
		(void)uu_unlock(uucplock);
		exit(3);
	}
	setbuf(stdout, NULL);
	vinit();
	setparity("none");
	boolean(value(VERBOSE)) = 0;

	if (connectit()) {
		printf("Connect failed\n");
		(void)uu_unlock(uucplock);
		exit(1);
	}
}
