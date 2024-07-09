/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI reset.c,v 1.3 1996/12/03 18:18:01 prb Exp
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <libdialer.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
dialer_reset(char *option)
{
    	char buf[256];
	struct sockaddr_un sun;
	int fd;

	if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0)
		return(-1);

	sun.sun_family = PF_LOCAL;
	strcpy(sun.sun_path, _PATH_DIALER);

	if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) < 0) {
		close(fd);
		return(-1);
	}

	strcpy(buf, RESOURCE_RESET ":");
	if (option) {
		strncat(buf, option, sizeof(buf));
		strncat(buf, ":\n", sizeof(buf));
	} else
		strncat(buf, "\n", sizeof(buf));
	errno = 0;
    	if (write(fd, buf, strlen(buf)) != strlen(buf)) {
		if (errno == 0)
			errno = ENOSPC;
		close(fd);
		return(-1);
	}

    	return(fd);
}
