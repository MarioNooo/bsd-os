/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI status.c,v 1.2 1996/06/14 00:46:27 prb Exp
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
dialer_status(char *cap)
{
	struct sockaddr_un sun;
	int fd;

	if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0)
		return(-1);

	sun.sun_family = PF_LOCAL;
	strcpy(sun.sun_path, _PATH_DIALER);

	if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) < 0)
		return(-1);
    	write(fd, RESOURCE_STATUS":", sizeof(RESOURCE_STATUS));
	if (cap)
		write(fd, cap, strlen(cap));
	write(fd, "\n", 1);
    	return(fd);
}
