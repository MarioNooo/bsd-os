/*	BSDI bidir.c,v 2.2 1996/03/29 03:05:55 prb Exp	*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#include "../../usr.bin/tip/pathnames.h"
#include "extern.h"

int
test_clocal(fd)
	int fd;
{
	struct termios t;

	tcgetattr(fd, &t);
	if (t.c_cflag & CLOCAL) {
		t.c_cflag &= ~CLOCAL;
		tcsetattr(fd, TCSANOW, &t);
		return (1);
	}

	return (0);
}

void
set_clocal(fd, t)
	int fd;
	struct termios *t;
{

	t->c_cflag |= CLOCAL;
	(void) tcsetattr(fd, TCSANOW, t);
}

void
flush_echos(fd)
	int fd;
{
	struct termios t;

	tcgetattr(fd, &t);
	tcsetattr(fd, TCSADRAIN, &t);
	usleep(250000);
	tcflush(fd, TCIOFLUSH);
}

char lockname[sizeof(_PATH_LOCKDIRNAME) + MAXNAMLEN];

int
seize_lock(ttyname, block)
	char *ttyname;
	int block;
{

	while (uu_lock(ttyname)) {
		if (!block)
			return (-1);
		sleep(15);
	}

	return (0);
}

void
release_lock(ttyname)
	char *ttyname;
{
	uu_unlock(ttyname);
}
