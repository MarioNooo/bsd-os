/*	BSDI acu.c,v 2.6 1998/04/13 23:08:12 chrisk Exp	*/

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
static char sccsid[] = "@(#)acu.c	5.8 (Berkeley) 3/2/91";
#endif /* not lint */

#include "tip.h"
#include <ctype.h>
#include <libdialer.h>

static void acuabort();
static jmp_buf jmpbuf;
static int gettyd = -1;		/* connection to gettyd */

/*
 * Establish connection for tip
 *
 * If DU is true, we should dial an ACU whose type is AT.
 * The phone numbers are in PN, and the call unit is in CU.
 *
 * If the PN is an '@', then we consult the PHONES file for
 *   the phone numbers.  This file is /etc/phones, unless overriden
 *   by an exported shell variable.
 *
 * The data base files must be in the format:
 *	host-name[ \t]*phone-number
 *   with the possibility of multiple phone numbers
 *   for a single host acting as a rotary (in the order
 *   found in the file).
 */
char *
connectit()
{
	cap_t *req = NULL;
	char *cp = PN;
	char string[256];

	if (!DU) {		/* regular connect message */
		if (BR <= 0)
			BR = DEFBR;
		if ((cp = hunt()) == NULL)
			return ("no available lines");
		if (FD < 0 && (FD = open(cp, O_RDWR | O_NONBLOCK)) >= 0) {
			set_clocal(FD);
			set_hupcl(FD);
			fcntl(FD, F_SETFL, 0); /* clear O_NONBLOCK */
			set_exclusive(FD);
		}
		if (FD < 0)
			return (strerror(errno));
		remote_ttysetup(FD, BR, MS);
		if (CM != NOSTR)
			pwrite(FD, CM, size(CM));
		return (NOSTR);
	}

	signal(SIGINT, acuabort);
	signal(SIGQUIT, acuabort);
	if (setjmp(jmpbuf)) {
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		printf("\ncall aborted\n");
		if (gettyd >= 0) {
			boolean(value(VERBOSE)) = FALSE;
			disconnect(NOSTR);
		}
		return ("interrupt");
	}

	if (vflag || boolean(value(GDEBUG)))
		cap_add(&req, "verbose", NULL, CM_NORMAL);
		
	if (DV != NOSTR)
		cap_add(&req, RESOURCE_LINE, DV, CM_NORMAL);

	if (BR > 0) {
		snprintf(string, sizeof(string), "%d", BR);
		cap_add(&req, RESOURCE_DTE, string, CM_NORMAL);
	}

	if (cp) {
		if (cp[0] == '@' && cp[1] == '\0') {
			snprintf(string, sizeof(string), "@%s", value(HOST));
			cap_add(&req, "number", string, CM_NORMAL);
		} else
			cap_add(&req, "number", cp, CM_NORMAL);
	}

	gettyd = connect_daemon(req, (uid_t)-1, (gid_t)-1);
	cap_free(req);

	if (gettyd >= 0) {
		while ((FD = connect_modem(&gettyd, &cp)) >= 0) {
			if (cp == NULL) {
				remote_ttysetup(FD, BR, MS);
				if (CM != NOSTR)
					pwrite(FD, CM, size(CM));
				return (NOSTR);
			}
			printf("%s\n", cp);
		}
		close(gettyd);
		gettyd = -1;
	} else
		cp = "call failed";

	return (cp);
}

disconnect(reason)
	char *reason;
{
	if (gettyd < 0)
		return;

	if (reason == NOSTR && boolean(value(VERBOSE)))
		printf("\r\ndisconnecting...");

	close(gettyd);
	gettyd = -1;
	close(FD);
	FD = -1;
}

static void
acuabort(s)
{
	signal(s, SIG_IGN);
	longjmp(jmpbuf, 1);
}
