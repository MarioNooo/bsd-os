/*-
 * Copyright (c) 1996,1998 Berkeley Software Design, Inc. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this notice is retained,
 * the conditions in the following notices are met, and terms applying
 * to contributors in the following notices also apply to Berkeley
 * Software Design, Inc.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *      Berkeley Software Design, Inc.
 * 4. Neither the name of the Berkeley Software Design, Inc. nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      BSDI auth.c,v 1.1 1998/01/15 02:29:14 prb Exp
 */
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>

#include <syslog.h>

#include <login_cap.h>
#ifdef	AUTH_PWEXPIRED
#include <stdarg.h>
#include <bsd_auth.h>
#endif

#include <netinet/in.h>
#include "ftpd.h"

extern int daemon_mode;
int ext_auth = 0;
login_cap_t *class = NULL;
static char *challenge = NULL;

char *
start_auth(char *style, char *name, struct passwd *pwd)
{
	int s;

	ext_auth = 1;	/* authentication is always external */

	if (challenge)
		free(challenge);
	challenge = NULL;

	if (!(class = login_getclass(pwd ? pwd->pw_class : 0)))
		return (NULL);

	if (pwd && pwd->pw_passwd[0] == '\0')
		return (NULL);

	if ((style = login_getstyle(class, style, "auth-ftp")) == NULL)
		return (NULL);

	if (auth_check(name, class->lc_class, style, "challenge", &s) < 0)
		return (NULL);

	if ((s & AUTH_CHALLENGE) == 0)
		return (NULL);

	challenge = auth_value("challenge");
	return (challenge);
}

char *
check_auth(char *name, char *passwd)
{
	char *e;
	int r;
#ifdef	AUTH_PWEXPIRED
	char number[16];
	auth_session_t *as;
#endif

	if (ext_auth == 0)
		return("Login incorrect.");
	ext_auth = 0;

	r = auth_response(name, class->lc_class, class->lc_style, "response",
	    NULL, challenge ? challenge : "", passwd);

	if (challenge)
		free(challenge);
	challenge = NULL;

	if (r <= 0) {
		e = auth_value("errormsg");
		return (e ? e : "Login incorrect.");
	}

#ifdef	AUTH_PWEXPIRED
	if ((as = auth_open()) == NULL)
		return("Approval failure.");
	snprintf(number, sizeof(number), "%d", OP(sessions));
	auth_setoption(as, "FTPD_HOST", OP(hostname));
	auth_setoption(as, "FTPD_SESSIONS", number);
	r = auth_approval(as, class, name, "ftp");
	auth_close(as);
#else
	r = auth_approve(class, name, "ftp");
#endif
	if (!r) {
		syslog(LOG_INFO|LOG_AUTH,
		    "FTP LOGIN FAILED (HOST) as %s: approval failure.", name);
		return("Approval failure.");
	}


    	return (NULL);
}
