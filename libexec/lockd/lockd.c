/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI lockd.c,v 1.1 1998/03/19 20:18:09 don Exp
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
#include <sys/socket.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <paths.h>

#include "nlm_prot.h"
#include "lockd.h"

void init_nsm(void);
void nlm_prog_1(struct svc_req *, SVCXPRT *);
void nlm_prog_3(struct svc_req *, SVCXPRT *);
void nlm_prog_4(struct svc_req *, SVCXPRT *);
void usage(void);
void svc_runx(int (*on_inter_f)(), void (*on_after_f)());
int nlm_interrupt();
void nlm_after();


int check_grace;
time_t grace_end;
DB *ownerdb;
DB *hostdb;
char my_hostname[MAXHOSTNAMELEN+1];
struct mon mon_host;
gid_t nogroup;
uid_t nobody;
uid_t daemon_uid;
pid_t client_pid = -1;

int d_args;
int d_cache;
int d_calls;
int d_bg;
int nsm_state;

#define _PATH_LOCKD_PID		_PATH_VARRUN "lockd.pid"

void
dumpall(sig, code)
	int sig;
	int code;
{
	dump_hostdb();
	dump_ownerdb();
}

void
cleanup(sig, code)
	int sig;
	int code;
{
	enum clnt_stat ret;
	my_id id;
	sm_stat stat;

#ifdef RUN_AS_DAEMON
	(void)seteuid(0);
#endif /* RUN_AS_DAEMON */

	/* if we get another signal, give up */
	signal(SIGHUP, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	if (client_pid != -1)
		kill(client_pid, sig);

	(void)pmap_unset(NLM_PROG, NLM_VERS);
	(void)pmap_unset(NLM_PROG, NLM_VERSX);
	(void)pmap_unset(NLM_PROG, NLM4_VERS);

	memset(&id, 0, sizeof(id));
	id.my_name = "NFS NLM";

	if ((ret = callrpc("localhost", SM_PROG, SM_VERS, SM_UNMON_ALL, 
				    xdr_my_id, &id, xdr_sm_stat, &stat)) != 0) {
		warnx("%lu %s", SM_PROG, clnt_sperrno(ret));
	}

	(void)unlink(_PATH_LOCKD_PID);
	exit(-1);
}

int
main(int argc, char *argv[])
{
	SVCXPRT *transp;
	int ch, debug, grace_period;
	char *p;
	struct passwd *pwd;
	struct group *grp;
	FILE *fp;

	debug = 0;
	grace_period = 2;
	while ((ch = getopt(argc, argv, "d:g:")) != EOF)
		switch (ch) {
		case 'd':
			for (p = optarg; *p != '\0'; ++p)
				switch (*p) {
				case 'A':	/* Log everything. */
					debug = d_args = d_cache = d_calls = 
						d_bg = 1;
					break;
				case 'a':	/* Log arguments. */
					debug = d_args = 1;
					break;
				case 'c':	/* Log cached host behavior. */
					debug = d_cache = 1;
					break;
				case 'r':	/* Log RPC calls. */
					debug = d_calls = 1;
					break;
				case 'b':	/* Log background activity. */
					debug = d_calls = 1;
					break;
				default:
					warnx("unknown debug flag: %c", *p);
					usage();
				}
			break;
		case 'g':
			if (!isdigit(optarg[0]) ||
			    (grace_period = atoi(optarg)) > 10)
				err(1, "illegal grace period: %s", optarg);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (gethostname(my_hostname, sizeof(my_hostname)) < 0) 
		err(1, "cannot get hostname");

#ifdef RUN_AS_DAEMON
	/*
	 * Get a uid to run under once we've done all the stuff we need to
	 * do as root.
	 */
	if ((pwd = getpwnam("daemon")) == NULL)
		err(1, "cannot get uid for user 'daemon'");
	daemon_uid = pwd->pw_uid;
#endif /* RUN_AS_DAEMON */

	/*
	 * Get a uid and gid for when AUTH_NULL is provided in a test request.
	 * (DIGITAL).
	 */
	if ((pwd = getpwnam("nobody")) == NULL)
		err(1, "cannot get uid for user 'nobody'");
	nobody = pwd->pw_uid;

	if ((grp = getgrnam("nogroup")) == NULL)
		err(1, "cannot get gid for group 'nogroup'");
	nogroup = grp->gr_gid;

	signal(SIGHUP, cleanup);
	signal(SIGTERM, cleanup);

	/* We support versions 1, 3, and 4. */
	(void)pmap_unset(NLM_PROG, NLM_VERS);
	(void)pmap_unset(NLM_PROG, NLM_VERSX);
	(void)pmap_unset(NLM_PROG, NLM4_VERS);

	/* Create UDP service. */
	if ((transp = svcudp_create(RPC_ANYSOCK)) == NULL)
		err(1, "cannot create udp service");
	if (!svc_register(transp, NLM_PROG, NLM_VERS, nlm_prog_1, IPPROTO_UDP))
		err(1, "unable to register (NLM_PROG, NLM_VERS, udp)");
	if (!svc_register(transp, NLM_PROG, NLM_VERSX, nlm_prog_3, IPPROTO_UDP))
		err(1, "unable to register (NLM_PROG, NLM_VERSX, udp)");
	if (!svc_register(transp, NLM_PROG, NLM4_VERS, nlm_prog_4, IPPROTO_UDP))
		err(1, "unable to register (NLM_PROG, NLM4_VERS, udp)");

	/* Create TCP service. */
	if ((transp = svctcp_create(RPC_ANYSOCK, 0, 0)) == NULL)
		err(1, "cannot create tcp service");
	if (!svc_register(transp, NLM_PROG, NLM_VERS, nlm_prog_1, IPPROTO_TCP))
		err(1, "unable to register (NLM_PROG, NLM_VERS, tcp)");
	if (!svc_register(transp, NLM_PROG, NLM_VERSX, nlm_prog_3, IPPROTO_TCP))
		err(1, "unable to register (NLM_PROG, NLM_VERSX, tcp)");
	if (!svc_register(transp, NLM_PROG, NLM4_VERS, nlm_prog_4, IPPROTO_TCP))
		err(1, "unable to register (NLM_PROG, NLM4_VERS, tcp)");

	if (debug == 0) {
		openlog(NULL, LOG_PID, LOG_DAEMON);
		syslog(LOG_INFO, "running");
	} else {
		openlog(NULL, LOG_PID | LOG_PERROR, LOG_DAEMON);
		syslog(LOG_INFO, "running: (debug)");
	}

	/*
	 * Do NOT to run this program from inetd -- the protocol assumes that
	 * it will run immediately at boot time... and beside, it is needed
	 * for the local side of NFS locking requests.
	 */
	if (debug == 0 && daemon(0, 0)) {
		syslog(LOG_ERR, "daemon: %m");
		err(1, "daemon");
	}

	if ((fp = fopen(_PATH_LOCKD_PID, "w")) != NULL) {
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
	}


	/* Contact the NSM. */
	init_nsm();

	/*
	 * For various reasons, mostly because it was the fastest way from
	 * point A to point B, we do all our locking in the kernel.  There
	 * are a set of system calls that we use for passing "server" lock
	 * requests into the kernel, and a set of system calls for passing
	 * "client" lock answers into the kernel.  The missing piece is a
	 * way for the kernel to forward "client" lock requests to us.  We
	 * do this with a FIFO that we create and which the kernel knows
	 * about.  Someone ought to fix it all up to use pipes, at least.
	 */
	client_pid = client_request();

	signal(SIGUSR1, dumpall);
	signal(SIGALRM, alarm_handler);

	/*
	 * Create a database to map host/pid pairs to owner descriptors.
	 */
	ownerdb = dbopen(NULL, O_RDWR, 0, DB_HASH, NULL);
	if (ownerdb == NULL) {
		syslog(LOG_ERR, "dbopen: %m");
		err(1, "dbopen");
	}

	/*
	 * Create a database to map host names to it's list of owners.
	 */
	hostdb = dbopen(NULL, O_RDWR, 0, DB_HASH, NULL);
	if (hostdb == NULL) {
		syslog(LOG_ERR, "dbopen: %m");
		err(1, "dbopen");
	}
	/* Initialize the grace period. */
	(void)time(&grace_end);
	grace_end += grace_period * SECSPERMIN;
	check_grace = TRUE;

#ifdef RUN_AS_DAEMON
	/* drop our root priviledges */
	(void)seteuid(daemon_uid);
#endif /* RUN_AS_DAEMON */

	svc_runx(nlm_interrupt, nlm_after);

	/* NOTREACHED */
#ifdef RUN_AS_DAEMON
	(void)seteuid(0);
#endif /* RUN_AS_DAEMON */
	(void)unlink(_PATH_LOCKD_PID);
	exit(1);
}

/*
 * init_nsm --
 *	Reset the NSM state-of-the-world and acquire its state.
 */
void
init_nsm(void)
{
	enum clnt_stat ret;
	my_id id;
	sm_stat stat;

	/*
	 * !!!
	 * The my_id structure isn't used by the SM_UNMON_ALL call, as far
	 * as I know.  Leave it empty for now.
	 */
	memset(&id, 0, sizeof(id));
	id.my_name = "NFS NLM";

	/*
	 * !!!
	 * The statd program must already be registered when lockd runs.
	 */
	do {
		ret = callrpc("localhost", SM_PROG, SM_VERS, SM_UNMON_ALL,
		    xdr_my_id, &id, xdr_sm_stat, &stat);
		if (ret == RPC_PROGUNAVAIL) {
			warnx("%lu %s", SM_PROG, clnt_sperrno(ret));
			sleep(2);
			continue;
		}
		break;
	} while (0);

	if (ret != 0) {
		errx(1, "%lu %s", SM_PROG, clnt_sperrno(ret));
	}

	nsm_state = stat.state;

	/* setup constant data for SM_MON calls */
	mon_host.mon_id.my_id.my_name = "localhost";
	mon_host.mon_id.my_id.my_prog = NLM_PROG;
	mon_host.mon_id.my_id.my_vers = NLM_VERS;
	mon_host.mon_id.my_id.my_proc = NLM_NSM_CALLBACK;  /* bsdi addition */

	if (d_calls)
		syslog(LOG_DEBUG, "NSM: current state: %d", stat.state);
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: lockd [-d Aacr] [-g minutes]\n");
	exit(1);
}
