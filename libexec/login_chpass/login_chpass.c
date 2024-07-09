/*-
 * Copyright (c) 1995,1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login_chpass.c,v 1.6 2003/08/21 22:29:27 polk Exp
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <stdarg.h>
#include <login_cap.h>

#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>

#define	INITIAL_TICKET	"krbtgt"

FILE *back;

#define	_PATH_LOGIN_LCHPASS	"/usr/libexec/login_lchpass"

static void _auth_spool(char *fmt, ...);
int	 klogin(struct passwd *, char *, char *, char *, char *);

int	notickets = 1;
char	*krbtkfile_env = 0;
int	authok;
int	always_use_klogin = 0;

int
main(argc, argv)
	int argc;
	char *argv[];
{
    	struct passwd *pwd;
    	char passbuf[80];
	char *p, *salt;
	int rval = 1;
	char localhost[MAXHOSTNAMELEN];
	char tkt_location[MAXPATHLEN];
	char realm[REALM_SZ];
    	char *class = 0;
    	char *username = 0;
    	char *instance;
    	int c;
    	struct rlimit rl;

	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rl);

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog("login", LOG_ODELAY, LOG_AUTH);

	if (gethostname(localhost, sizeof(localhost)) < 0)
		syslog(LOG_ERR, "couldn't get local hostname: %m");

    	while ((c = getopt(argc, argv, "s:v:")) != EOF)
		switch(c) {
		case 'v':
			break;
		case 's':	/* service */
			if (strcmp(optarg, "login") != 0) {
				syslog(LOG_ERR, "%s: invalid service", optarg);
				exit(1);
			}
			break;
		default:
			syslog(LOG_ERR, "usage error");
			exit(1);
		}

	switch(argc - optind) {
	case 2:
		class = argv[optind + 1];
	case 1:
		username = argv[optind];
		break;
	default:
		syslog(LOG_ERR, "usage error");
		exit(1);
	}

	instance = strchr(username, '.');
	if (instance)
		*instance++ = '\0';
	else
		instance = "";
	
	if (krb_configured() == KFAILURE)
		goto use_lchpass;

	if (krb_get_lrealm(realm, 0) != KSUCCESS)
		goto use_lchpass;

	snprintf(tkt_location, sizeof(tkt_location), "%s.chpass.%s.%d",
	    TKT_ROOT, username, getpid());
	krb_set_tkt_string(tkt_location);
	
	c = krb_get_pw_in_tkt(username, instance, realm, INITIAL_TICKET,
	    realm, DEFAULT_TKT_LIFE, "-");
	dest_tkt();
	unlink(tkt_location);	/* just to make sure */

	if (c == KDC_PR_UNKNOWN) {
use_lchpass:
		if (*instance)
			*--instance = '.';
		argv[0] = strrchr(_PATH_LOGIN_LCHPASS, '/') + 1;
		execv(_PATH_LOGIN_LCHPASS, argv);
		syslog(LOG_ERR, "%s: %s", _PATH_LOGIN_LCHPASS, strerror(errno));
		exit(1);
	}

	if (!(back = fdopen(3, "a")))  {
		syslog(LOG_ERR, "reopening back channel");
		exit(1);
	}

	pwd = getpwnam(username);

	(void)setpriority(PRIO_PROCESS, 0, -4);

	p = getpass("Kerberos Password:");

	if (pwd)
		rval = klogin(pwd, instance, localhost, p, NULL);


	if (!pwd || rval) {
		memset(p, 0, strlen(p));
		exit(1);
	}

	if (krbtkfile_env)
		setenv("KRBTKFILE", krbtkfile_env, 1);
	rval = krb_passwd(p);
	memset(p, 0, strlen(p));
	if (krbtkfile_env)
		unlink(krbtkfile_env);
	dest_tkt();
#ifdef	BI_SILENT
	if (rval == 0)
		_auth_spool(BI_SILENT "\n");
#endif
    	return(rval);
}

static void
_auth_spool(char *fmt, ...)
{
	va_list ap;

	if (back) {
		va_start(ap, fmt);

		vfprintf(back, fmt, ap);

		va_end(ap);
	}
}
