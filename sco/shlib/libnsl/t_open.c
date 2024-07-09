/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_open.c,v 2.1 1995/02/03 15:20:10 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

/*
 * NOT YET	Make sure file descriptor isn't on queue.
 *		I think this is only a small memory leak problem.
 */

int
t_open(path, oflag, ip)
	char *path;
	int oflag;
	t_info_t *ip;
{
	int protocol;
	int fd;
	int i;
	ts_t *tsp;

	DBG_ENTER(("t_open", pstring, path, phex, &oflag, 0));

	if (!t_nerr)
		tli_initerr();

	if (strcmp(path, "/dev/inet/tcp") == 0)
		protocol = SOCK_STREAM; 
	else if (strcmp(path, "/dev/inet/udp") == 0)
		protocol = SOCK_DGRAM; 
	else {
		t_errno = TNOTSUPPORT;
		DBG_EXIT(("t_open TNOTSUPPORT", -1, pstring, path, 0));
		return (-1);
	}
	fd = socket(AF_INET, protocol, 0);
	if (fd < 0) {
		t_errno = TSYSERR;
		tli_maperrno();
		DBG_EXIT(("t_open", -1, 0));
		return (-1);
	}
	if ((tsp = memalloc(sizeof(*tsp))) == NULL)
		return (-1);

	tsp->ts_fd = fd;
	tsp->ts_domain = PF_INET;	/* future unix domain ?? */
	tsp->ts_protocol = protocol;
	tsp->ts_next = ts_list;
	tsp->ts_state = T_UNBND;
	ts_list = tsp;
	
	if (ip)
		t__getinfo(tsp, ip);

	i = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof (i)))
	    phex("setsockopt failed", &errno);

	DBG_EXIT(("t_open", fd, pt_info, ip, 0));
	return (fd);
}
