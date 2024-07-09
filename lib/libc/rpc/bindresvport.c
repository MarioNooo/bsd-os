/*	BSDI bindresvport.c,v 2.4 2003/02/05 23:34:20 polk Exp	*/

static  char sccsid[] = "@(#)bindresvport.c	2.2 88/07/29 4.0 RPCSRC 1.8 88/02/08 SMI";
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <bitstring.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STARTPORT 600
#define ENDPORT (IPPORT_RESERVED - 1)
#define NPORTS	(ENDPORT - STARTPORT + 1)

#define	_PATH_RESVPORTS	_PATH_VARRUN"resvports"

static bitstr_t bit_decl(resvports, IPPORT_RESERVED);
static struct stat last_sb;

/* 
 * Read the reserved ports file.  Fail if anything looks out of place.
 * We try to use locking, but will retry if it is not supported.
 */
static void
read_resvports()
{
	struct stat sb;
	bitstr_t bit_decl(newports, IPPORT_RESERVED);
	size_t len;
	int fd;
	static int flags = O_RDONLY|O_SHLOCK;
	FILE *fp;
	char *line;
	char *ep;

 Retry:
	/* Try to use locking, but do not fail if we are unable to */
	if ((fd = open(_PATH_RESVPORTS, flags, 0)) < 0) {
		if (errno == EOPNOTSUPP && flags & O_SHLOCK) {
			flags &= ~O_SHLOCK;
			goto Retry;
		}
		return;
	}

	fp = fdopen(fd, "r");

	if (fstat(fd, &sb) < 0)
		goto Return;

	/* Ignore  if it is not newer than the last one we read */
	if (sb.st_mtime <= last_sb.st_mtime)
		goto Return;

	bit_nclear(newports, 0, 1023);

	/* Read a line at a time */
	while ((line = fgetln(fp, &len)) != NULL) {
		u_long first_port;
		u_long last_port;

		/* Trim trailing newline */
		if (line[len-1] == '\n')
			line[--len] = 0;
		if (*line == 0)
			continue;

		first_port = strtoul(line, &ep, 10);
		/* Bail if anything looks wrong */
		if ((first_port == ULONG_MAX && errno == ERANGE) ||
		    first_port < 1 || first_port > USHRT_MAX ||
		    line == ep)
			goto Return;
		/* Ignore if it is not in our range */
		if (first_port > ENDPORT)
			continue;
		if (*ep == '-') {
			line = ep + 1;
			last_port = strtoul(line, &ep, 10);
			/* Bail if anything looks wrong */
			if ((last_port == ULONG_MAX && errno == ERANGE) ||
			    last_port < 1 || last_port > USHRT_MAX ||
			    last_port <= first_port ||
			    line == ep || *ep != 0)
				goto Return;
			if (last_port > ENDPORT)
				last_port = ENDPORT;
			bit_nset(newports, first_port, last_port);
		} else
			bit_set(newports, first_port);
	}

	/* Use the new copy and remember the timestamp on the file */
	bcopy(newports, resvports, sizeof(resvports));
	bcopy(&sb, &last_sb, sizeof(last_sb));

 Return:
	fclose(fp);

	return;
}

/*
 * Bind a socket to a privileged IP port
 */
bindresvport(sd, sin)
	int sd;
	struct sockaddr_in *sin;
{
	static short port;
	struct sockaddr_in myaddr;
	struct servent *sp;
	size_t optlen;
	int i;
	int res;
	char *proto;

	if (sin == (struct sockaddr_in *)0) {
		sin = &myaddr;
		bzero(sin, sizeof (*sin));
		sin->sin_family = AF_INET;
	} else if (sin->sin_family != AF_INET) {
		errno = EPFNOSUPPORT;
		return (-1);
	}

	/* Determine the protcol for the socket */
	i = sizeof(res);
	if (getsockopt(sd, SOL_SOCKET, SO_TYPE, &res, &i) < 0) {
		return (-1);
	}
	switch (res) {
	case SOCK_STREAM:
		proto = "tcp";
		break;
	case SOCK_DGRAM:
		proto = "udp";
		break;
	default:
		errno = EPFNOSUPPORT;
		return (-1);
        }

	read_resvports();

	if (port == 0) {
		port = (getpid() % NPORTS) + STARTPORT - 1;
	}
	res = -1;
	errno = EADDRINUSE;
	for (i = 0; i < NPORTS && res < 0 && errno == EADDRINUSE; i++) {
		if (++port > ENDPORT) {
			port = STARTPORT;
		}
		sin->sin_port = htons(port);
		/* Skip well-known ports */
		if (bit_test(resvports, port))
			continue;

		res = bind(sd, (struct sockaddr *)sin, sizeof(*sin));
	}
	return (res);
}
