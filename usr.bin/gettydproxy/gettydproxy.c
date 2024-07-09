/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gettydproxy.c,v 1.7 1998/08/13 22:47:49 chrisk Exp
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <libdialer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Earlier versions inadvertantly used only %MODEM rather then %MODEM%. To
 * be backwards compatible to any scripts people may have written we have
 * to check for both. You need to be sure and check for the longer version
 * first.
 */

#define	OLDMODEM	"%MODEM"
#define	OLDMLEN		(sizeof(OLDMODEM) - 1)
#define	MODEM		"%MODEM%"
#define	MLEN		(sizeof(MODEM) - 1)

#define	PREFIX	RESOURCE_REQUEST ":"
#define	PLEN	(sizeof(PREFIX) - 1)

void
main(int argc, char *argv[])
{
	struct stat sb;
	int c, daemon_fd, dflag, fd, vflag;
	cap_t *req = NULL;
	char *e, cmdbuf[8192], device[16];

	dflag = vflag = 0;
	while ((c = getopt(argc, argv, "do:v")) != EOF)
		switch (c) {
		case 'd':
			dflag = 1;
			break;
		case 'o':
			if ((e = strchr(optarg, '='))) {
				*e++ = '\0';
				if (*e == '\0')
					e = NULL;
			}
			if (cap_add(&req, optarg, e, CM_NORMAL))
				err(1, NULL);
			break;
		case 'v':
			vflag = 1;
			break;
		default:
usage:			fprintf(stderr,
	    "usage: gettydproxy [-dv] [-o option[=value]] command ...\n");
			exit(1);
		}

	if (optind >= argc)
		goto usage;

	if (vflag)
		snprintf(device, sizeof(device), "%s", MODEM);
	else {
		if ((daemon_fd = connect_daemon(req, (uid_t)-1, (gid_t)-1)) < 0)
			err(1, NULL);

		e = NULL;

		if ((fd = connect_modem(&daemon_fd, &e)) < 0)
			errx(1, e);

		if (dflag) {
			if (fstat(fd, &sb) < 0)
				err(1, NULL);
			close(fd);
			e = devname(sb.st_rdev, sb.st_mode & S_IFMT);
			if (e == NULL)
				errx(1,
				    "Could not find device in device database");
			snprintf(device, sizeof(device), "/dev/%s", e);
		} else
			snprintf(device, sizeof(device), "/dev/fd/%d", fd);
	}

	e = cmdbuf; 
	while (optind < argc) {
		char *v = argv[optind++];
		char *r;
		if (e != cmdbuf)
			*e++ = ' ';
		while (*v != '\0' && ((r = strstr(v, MODEM)) != NULL ||
		    (r = strstr(v, OLDMODEM)) != NULL)) {
			if (r != v) {
				if (e + (r - v) >= &cmdbuf[sizeof(cmdbuf)-1])
					errx(1, "command too long");
				strncpy(e, v, r - v);
				e += r - v;
			}

			if (strncmp(r,MODEM,MLEN) == 0)
				v = r + MLEN;
			else
				v = r + OLDMLEN;

			if (e + strlen(device) >= &cmdbuf[sizeof(cmdbuf)-1])
				errx(1, "command too long");
			strcpy(e, device);
			e += strlen(device);
		}
		if (*v) {
			if (e + strlen(v) >= &cmdbuf[sizeof(cmdbuf)-1])
				errx(1, "command too long");
			strcpy(e, v);
			e += strlen(v);
		}
	}
	*e = NULL;

	if (vflag) {
		char buf[2048];
		strcpy(buf, PREFIX);
		if (cap_dump(req, buf + PLEN, sizeof(buf) - PLEN) == NULL)
			errx(1, "out of space");
		printf("request to gettyd : %s", buf);
		printf("command to execute: %s\n", cmdbuf);
		exit(0);
	}
	exit(system(cmdbuf));
}
