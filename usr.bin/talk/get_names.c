/*
 * Copyright (c) 1983, 1993
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
static char sccsid[] = "@(#)get_names.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>

#include <protocols/talkd.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>

#include "talk.h"

extern	CTL_MSG msg;

#define	LEN_MSG	"\"%s\" truncated to \"%s\", max length is %u characters"

/*
 * Determine the local and remote user, tty, and machines
 */
get_names(argc, argv)
	int argc;
	char *argv[];
{
	char hostname[MAXHOSTNAMELEN];
	char *his_name, *my_name;
	char *my_machine_name, *his_machine_name;
	char *my_tty, *his_tty;
	char *names;
	register char *cp;
	char line[sizeof(LEN_MSG) + sizeof(msg.l_name) + sizeof(msg.r_name) + 5];

	if (argc < 2 ) {
		printf("Usage: talk user [ttyname]\n");
		exit(-1);
	}
	if (!isatty(0)) {
		printf("Standard input must be a tty, not a pipe or a file\n");
		exit(-1);
	}
	if ((my_name = getlogin()) == NULL) {
		struct passwd *pw;

		if ((pw = getpwuid(getuid())) == NULL) {
			printf("You don't exist. Go away.\n");
			exit(-1);
		}
		my_name = pw->pw_name;
	}
	gethostname(hostname, sizeof (hostname));
	my_machine_name = hostname;
	names = strdup(argv[1]);
	/* check for, and strip out, the machine name of the target */
	cp = strpbrk(names, "@:!.");
	if (cp == NULL) {
		/* this is a local to local talk */
		his_name = names;
		his_machine_name = my_machine_name;
	} else {
		if (*cp++ == '@') {
			/* user@host */
			his_name = names;
			his_machine_name = cp;
		} else {
			/* host.user or host!user or host:user */
			his_name = cp;
			his_machine_name = names;
		}
		*--cp = '\0';
	}
	if (argc > 2)
		his_tty = argv[2];	/* tty name is arg 2 */
	else
		his_tty = "";
	get_addrs(my_machine_name, his_machine_name);
	/*
	 * Initialize the message template.
	 */
	msg.vers = TALK_VERSION;
	msg.addr.sa_family = htons(AF_INET);
	msg.ctl_addr.sa_family = htons(AF_INET);
	msg.id_num = htonl(0);
	strncpy(msg.l_name, my_name, sizeof(msg.l_name));
	msg.l_name[sizeof(msg.l_name) - 1] = '\0';
	if (strcmp(msg.l_name, my_name) != 0) {
		(void)snprintf(line, sizeof(line), LEN_MSG, my_name, 
		    msg.l_name, sizeof(msg.l_name) - 1);
		msg_add(line);
	}
	strncpy(msg.r_name, his_name, sizeof(msg.r_name));
	msg.r_name[sizeof(msg.r_name) - 1] = '\0';
	if (strcmp(msg.r_name, his_name) != 0) {
		(void)snprintf(line, sizeof(line), LEN_MSG, his_name,
		    msg.r_name, sizeof(msg.r_name) - 1);
		msg_add(line);
	}
	strncpy(msg.r_tty, his_tty, sizeof(msg.r_tty));
	msg.r_tty[sizeof(msg.r_tty) - 1] = '\0';
}
