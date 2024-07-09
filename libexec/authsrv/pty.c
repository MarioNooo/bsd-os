/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI pty.c,v 1.2 1998/09/23 19:13:52 torek Exp
 */
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define _PATH_TTY       "/dev/tty"

int
GetPty(char *line)
{
	int p;

	char c, *p1, *p2;
	int i;

	sprintf(line, "/dev/ptyXX");
	p1 = &line[8];
	p2 = &line[9];

	for (c = 'p'; c <= 's'; c++) {
		struct stat stb;

		*p1 = c;
		*p2 = '0';
		if (stat(line, &stb) < 0)
			break;
		for (i = 0; i < 16; i++) {
			*p2 = "0123456789abcdef"[i];
			p = open(line, O_RDWR);
			if (p > 0) {
				line[5] = 't';
				return(p);
			}
		}
	}
	return(-1);
}

void
GetPtySlave(char *line, int fd3)
{
	struct termios tio;
	int t = -1;
	int NFiles;

        if ((NFiles = sysconf(_SC_OPEN_MAX)) < 1)
		NFiles = 20;

	if ((t = open(_PATH_TTY, O_RDWR)) >= 0) {
		ioctl(t, TIOCNOTTY, (char *)0);
		close(t);
	}

	chown(line, getuid(), getgid());
	chmod(line, 0600);

	signal(SIGHUP, SIG_IGN);
	signal(SIGHUP, SIG_DFL);

	if ((t = open(line, O_RDWR|O_NOCTTY)) < 0) {
		perror(line);
		exit(1);
	}

	tcgetattr(t, &tio);

	tio.c_iflag = TTYDEF_IFLAG;
	tio.c_oflag = TTYDEF_OFLAG;
	tio.c_lflag = TTYDEF_LFLAG;
	tio.c_cflag = TTYDEF_CFLAG;

	cfsetospeed(&tio, TTYDEF_SPEED);
	cfsetispeed(&tio, TTYDEF_SPEED);

	tcsetattr(t, TCSANOW, &tio);

	if (setsid() < 0) {
		perror("setsid");
		exit(1);
	}
	if (ioctl(t, TIOCSCTTY, 0) < 0)
		perror("TIOCSCTTY");

	if (t != 0)
		dup2(t, 0);
	if (t != 1)
		dup2(t, 1);
	if (t != 2)
		dup2(t, 2);
	if (fd3 != 3)
		dup2(fd3, 3);
	for (t = 4; t < NFiles; ++t)
		close(t);
	setgid(getgid());
	setuid(getuid());
	t = 1;
        ioctl(0, TIOCEXT, (char *)&t);
	return;
}
