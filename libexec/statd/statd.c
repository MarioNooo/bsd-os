/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI statd.c,v 1.1 1998/03/19 20:20:22 don Exp
 */
/*
 * Copyright (c) 1995
 *	A.R. Gordon (andrew.gordon@net-tel.co.uk).  All rights reserved.
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
 *	This product includes software developed for the FreeBSD project
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANDREW GORDON AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/types.h>
#include <sys/stat.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpcsvc/sm_inter.h>

#include <limits.h>
#include <db.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <paths.h>
#include <signal.h>

#include "statd.h"

void sm_prog_1(struct svc_req *, SVCXPRT *);
void usage(void);

int	 d_cache;
int	 d_calls;
DB 	*stat_db;
FILE	*statfp;

/*
 * We use the length + first 8 characters for the hash.
 */
u_int32_t
hostname_hash(const void *str, size_t len)
{
	int i = len > 8 ? 8 : len;
	u_int32_t hash;

	hash = len;
	
	while (i-- > 0) {
		hash += (u_int32_t)*((unsigned char *)str++);
	}
	return (hash);
}

void
cleanup(sig, code)
	int sig;
	int code;
{

	/* if we get another signal, give up */
	signal(SIGHUP, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	(void)pmap_unset(SM_PROG, SM_VERS);

	(void)unlink(_PATH_STATD_PID);
	exit(-1);
}

int
main(int argc, char *argv[])
{
	SVCXPRT *transp;
	int ch, debug;
	char *p;
	HASHINFO hashinfo;
	FILE *fp;

	debug = 0;
	while ((ch = getopt(argc, argv, "d:")) != EOF)
		switch (ch) {
		case 'd':
			for (p = optarg; *p != '\0'; ++p)
				switch (*p) {
				case 'A':	/* Log everything. */
					debug = d_cache = d_calls = 1;
					break;
				case 'c':	/* Log cached host behavior. */
					debug = d_cache = 1;
					break;
				case 'r':	/* Log RPC calls. */
					debug = d_calls = 1;
					break;
				default:
					warnx("unknown debug flag: %c", *p);
					usage();
				}
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	signal(SIGHUP, cleanup);
	signal(SIGTERM, cleanup);

	(void)pmap_unset(SM_PROG, SM_VERS);

	/* Create UDP service. */
	if ((transp = svcudp_create(RPC_ANYSOCK)) == NULL)
		err(1, "cannot create udp service");
	if (!svc_register(transp, SM_PROG, SM_VERS, sm_prog_1, IPPROTO_UDP))
		err(1, "unable to register (SM_PROG, SM_VERS, udp)");

	/* Create TCP service. */
	if ((transp = svctcp_create(RPC_ANYSOCK, 0, 0)) == NULL)
		err(1, "cannot create tcp service");
	if (!svc_register(transp, SM_PROG, SM_VERS, sm_prog_1, IPPROTO_TCP))
		err(1, "unable to register (SM_PROG, SM_VERS, tcp)");

	/*
	 * Do NOT to run this program from inetd -- the protocol assumes that
	 * it will run immediately at boot time.
	 */
	if (debug == 0) {
		openlog(NULL, 0, LOG_DAEMON);
		syslog(LOG_INFO, "running");
	} else {
		openlog(NULL, LOG_PID | LOG_PERROR, LOG_DAEMON);
		syslog(LOG_INFO, "running: (debug)");
	}

#ifndef SIMPLE
	hashinfo.bsize = 256; 
	hashinfo.ffactor = 8; 
	hashinfo.nelem = 1; 
	hashinfo.cachesize = 10*1024; 
	hashinfo.hash = hostname_hash; 
	hashinfo.lorder = 0; 

	stat_db = dbopen(NULL, O_RDWR, 0666, DB_HASH, &hashinfo);
	if (stat_db == NULL) {
		err(1, "unable to create memory based hash database");
	}
#endif

	/* Open the status file. */
	init_statfp(0);

	/* Update our status number. */
	(void)our_state(1);

	/* Send out the SM_NOTIFY calls. */
	send_sm_notify();

	/*
	 * Do NOT to run this program from inetd -- the protocol assumes that
	 * it will run immediately at boot time.
	 */
	if (debug == 0 && daemon(0, 0)) {
		syslog(LOG_ERR, "daemon: %m");
		err(1, "daemon");
	}

	if ((fp = fopen(_PATH_STATD_PID, "w")) != NULL) {
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
	}

	svc_run();

	/* NOTREACHED */
	(void)unlink(_PATH_STATD_PID);
	exit (1);
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: statd [-d Acr]\n");
	exit(1);
}
