/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI fb.c,v 1.3 1998/12/18 22:50:38 chrisk Exp
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "daemon.h"

extern fd_set *read_set, *write_set, *close_set;

int
fb_init(fb_data_t *fb, int fd)
{
	fb->fb_fd = fd;
	fb->fb_tail = fb->fb_buffer = malloc(fb->fb_bufsize = 1024);
	if (!fb->fb_buffer)
		return(-1);
	return(fd);
}

void
fb_close(fb_data_t *fb)
{
	if (fb->fb_buffer) {
		free(fb->fb_buffer);
		fb->fb_buffer = 0;
	}
	if (fb->fb_fd >= 0) {
	    close(fb->fb_fd);
	    FD_CLR(fb->fb_fd, read_set);
	    FD_CLR(fb->fb_fd, write_set);
	    FD_SET(fb->fb_fd, close_set);
	    fb->fb_fd = -1;
	}
}

int
fb_read(fb_data_t *fb, char term)
{
	int r;
	char c;

	while ((r = read(fb->fb_fd, &c, 1)) == 1)
		if (fb->fb_tail < fb->fb_buffer + fb->fb_bufsize) {
			if (c == 0 || c == term) {
				*fb->fb_tail = 0;
				fb->fb_tail = fb->fb_buffer;
				return(1);
			}
			*fb->fb_tail++ = c;
		} else
			return(-2);

	if (r == 0)
		return(-1);
	switch (errno) {
	case EINTR:
	case EAGAIN:
		return(0);
	default:
		return(-3);
	}
}
