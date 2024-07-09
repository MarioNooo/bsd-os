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
static char sccsid[] = "@(#)init_disp.c	8.2 (Berkeley) 2/16/94";
#endif /* not lint */

/*
 * Initialization code for the display package,
 * as well as the signal handling routines.
 */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/termios.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "talk.h"

struct msg_entry {
	char *msg_msg;
	TAILQ_ENTRY(msg_entry) msg_link;
};
TAILQ_HEAD(msg_head, msg_entry) msg_head = { NULL, &msg_head.tqh_first };

static void sig_sent __P((int));

/* 
 * Set up curses, catch the appropriate signals,
 * and build the various windows.
 */
init_display()
{
	struct sigaction sa;
	struct msg_entry *mp;

	if (initscr() == NULL)
		errx(1, "Terminal type unset or lacking necessary features.");
	(void) sigaction(SIGTSTP, NULL, &sa);
	sigaddset(&sa.sa_mask, SIGALRM);
	(void) sigaction(SIGTSTP, &sa, NULL);
	curses_initialized = 1;
	clear();
	refresh();
	noecho();
	crmode();
	signal(SIGINT, sig_sent);
	signal(SIGPIPE, sig_sent);
	/* curses takes care of ^Z */
	my_win.x_nlines = LINES / 2;
	my_win.x_ncols = COLS;
	my_win.x_win = newwin(my_win.x_nlines, my_win.x_ncols, 0, 0);
	scrollok(my_win.x_win, FALSE);
	wclear(my_win.x_win);

	his_win.x_nlines = LINES / 2 - 1;
	his_win.x_ncols = COLS;
	his_win.x_win = newwin(his_win.x_nlines, his_win.x_ncols,
	    my_win.x_nlines+1, 0);
	scrollok(his_win.x_win, FALSE);
	wclear(his_win.x_win);

	line_win = newwin(1, COLS, my_win.x_nlines, 0);
	box(line_win, '-', '-');
	wrefresh(line_win);
	/* let them know we are working on it */
	current_state = "No connection yet";

	/* Run the message queue */
	for (mp = TAILQ_FIRST(&msg_head); mp != TAILQ_END(&msg_head);
	     mp = TAILQ_NEXT(mp, msg_link)) {
		message(mp->msg_msg);
	}
}

/*
 * Trade edit characters with the other talk. By agreement
 * the first three characters each talk transmits after
 * connection are the three edit characters.
 */
set_edit_chars()
{
	char buf[3];
	int cc;
	struct termios tio;

	if (tcgetattr(0, &tio) != 0)
		p_error("Getting terminal attributes");
	if ((my_win.cerase = tio.c_cc[VERASE]) == (char)_POSIX_VDISABLE)
		my_win.cerase = CERASE;
	if ((my_win.kill = tio.c_cc[VKILL]) == (char)_POSIX_VDISABLE)
		my_win.kill = CKILL;
	if ((my_win.werase = tio.c_cc[VWERASE]) == (char)_POSIX_VDISABLE)
		my_win.werase = CWERASE;

	buf[0] = my_win.cerase;
	buf[1] = my_win.kill;
	buf[2] = my_win.werase;
	cc = write(sockt, buf, sizeof(buf));
	if (cc != sizeof(buf) )
		p_error("Lost the connection");
	cc = read(sockt, buf, sizeof(buf));
	if (cc != sizeof(buf) )
		p_error("Lost the connection");
	his_win.cerase = buf[0];
	his_win.kill = buf[1];
	his_win.werase = buf[2];
}

/*ARGSUSED*/
static void
sig_sent(signo)
     int signo;
{
	/* For slow terminals I assume */
	message("Connection closing. Exiting");
	quit(0, NULL);
}

/*
 * All done talking...hang up the phone and reset terminal thingy's
 */
quit(error, msg)
     int error;
     char *msg;
{
	struct msg_entry *mp;

	if (curses_initialized) {
		wmove(his_win.x_win, his_win.x_nlines-1, 0);
		wclrtoeol(his_win.x_win);
		wrefresh(his_win.x_win);
		endwin();
	}
	if (invitation_waiting)
		send_delete();

	/* Print any warning messages */
	for (mp = TAILQ_FIRST(&msg_head); mp != TAILQ_END(&msg_head);
	     mp = TAILQ_NEXT(mp, msg_link)) {
		warnx("%s", mp->msg_msg);
	}

	if (msg != NULL) {
		if ((errno = error) != 0)
			warn("%s", msg);
		else
			warnx("%s", msg);
	}
	exit(0);
}

/*
 * Add a message to the list of messages to be displayed at screen startup
 */
msg_add(char *msg)
{
	struct msg_entry *mp;
	
	if ((mp = calloc(1, sizeof(*mp))) == NULL)
		errx(1, "out of memory");

	if ((mp->msg_msg = strdup(msg)) == NULL)
		errx(1, "out of memory");

	TAILQ_INSERT_TAIL(&msg_head, mp, msg_link);
}
