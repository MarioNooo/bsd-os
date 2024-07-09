/* BSDI io.c,v 2.5 1999/01/21 03:15:59 jch Exp */
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
static char sccsid[] = "@(#)io.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/*
 * This file contains the I/O handling and the exchange of 
 * edit characters. This connection itself is established in
 * ctl.c
 */

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "talk.h"

#define A_LONG_TIME 10000000

/*
 * The routine to do the actual talking
 */
talk()
{
	fd_set read_template, read_set;
	int nb;
	char buf[BUFSIZ], *p;
	struct timeval wait;

#ifdef	NCURSES_VERSION
	message("Connection established");
	beep();
	beep();
	beep();
#else
	message("Connection established\007\007\007");
#endif
	current_line = 0;
	FD_ZERO(&read_template);
	FD_SET(fileno(stdin), &read_template);
	FD_SET(sockt, &read_template);

	for (;;) {
		FD_COPY(&read_template, &read_set);
		wait.tv_sec = A_LONG_TIME;
		wait.tv_usec = 0;
		nb = select(sockt + 1, &read_set, NULL, NULL, &wait);
		if (nb <= 0) {
			if (errno == EINTR) {
				read_set = read_template;
				continue;
			}
			/* panic, we don't know what happened */
			p_error("Unexpected error from select");
		}
		if (FD_ISSET(sockt, &read_set)) { 
			/* There is data on sockt */
			nb = read(sockt, buf, sizeof buf);
			if (nb == 0) {
				message("Connection closed: Exiting");
				quit(0, NULL);
			} 
			if (nb < 0)
				quit(errno, "Exiting");
			display(&his_win, buf, nb);
		}
		if (FD_ISSET(fileno(stdin), &read_set)) {
			/*
			 * We can't make the tty non_blocking, because
			 * curses's output routines would screw up
			 */
			ioctl(0, FIONREAD, &nb);
			if ((nb = read(0, buf, nb)) == -1)
				p_error("read: stdin");
			if (nb != 0) {
				/*
				 * Due to quirks of linemode telnet
				 * and the tty driver, we can receive
				 * '^M' rather than newlines.
				 * Translate here so they are displayed
				 * correctly both here and remotely.
				 */
				for (p = buf; p < buf + nb; ++p)
					if (*p == '\r')
						*p = '\n';
				display(&my_win, buf, nb);
				/*
				 * Might lose data here because socket is
				 * non-blocking.
				 */
				(void)write(sockt, buf, nb);
			}
		}
	}
}

/*
 * p_error prints the system error message on the standard location
 * on the screen and then exits. (i.e. a curses version of perror)
 */
p_error(string) 
	char *string;
{
	int error = errno;

	quit(error, string);
}

/*
 * Display string in the standard location
 */
message(string)
	char *string;
{
	wmove(my_win.x_win, current_line % my_win.x_nlines, 0);
	wprintw(my_win.x_win, "[%s]", string);
	wclrtoeol(my_win.x_win);
	current_line++;
	wmove(my_win.x_win, current_line % my_win.x_nlines, 0);
	wrefresh(my_win.x_win);
}
