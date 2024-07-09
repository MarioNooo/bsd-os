/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * Based on:
 *	BSDI login_kerberos.c,v 1.17 2001/10/03 17:29:55 polk Exp
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
#include <libpasswd.h>

#include <krb5.h>

FILE *back;

int	k5login(struct passwd *, char *, char *, char *);
int	k5oktologin(char *, char *, char *);
int	always_use_klogin = 0;

int		notickets = 1;
char		*krb5ccname_env = 0;
int		authok;
char		*progname;
krb5_context	context;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	krb5_error_code		code;
    	struct passwd *pwd;
	char *p, *class, *username, *instance, *wheel;
	char localhost[MAXHOSTNAMELEN], response[1024], *names[3];
    	int c, krb5_configed, mode, rval, lastchance;
    	struct rlimit rl;
	login_cap_t *lc;

	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rl);

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog(NULL, LOG_ODELAY, LOG_AUTH);

	if (gethostname(localhost, sizeof(localhost)) < 0)
		syslog(LOG_ERR, "couldn't get local hostname: %m");

	class = NULL;
	username = NULL;
	instance = NULL;
	wheel = NULL;
	mode = 0;
	rval = 1;
	back = NULL;
	p = NULL;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "dv:s:")) != EOF)
		switch(c) {
		case 'd':
			back = stdout;
			break;

		case 'v':
			if (strncmp(optarg, "wheel=", 6) == 0)
				wheel = optarg + 6;
			else if (strncmp(optarg, "lastchance=", 10) == 0)
				lastchance = strcmp(optarg + 10, "yes") == 0;
			break;
		case 's':	/* service */
			if (strcmp(optarg, "login") == 0)
				mode = 0;
			else if (strcmp(optarg, "challenge") == 0)
				mode = 1;
			else if (strcmp(optarg, "response") == 0)
				mode = 2;
			else {
				syslog(LOG_ERR, "invalid service: %s", optarg);
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

	instance = username;
	username = strsep(&instance, "./");
	if (!instance)
		instance = "";

	if (back == NULL && (back = fdopen(3, "r+")) == NULL)  {
		syslog(LOG_ERR, "reopening back channel");
		exit(1);
	}

	pwd = getpwnam(username);
	if (pwd)
		pwd = pw_copy(pwd);
	if (class && pwd)
		pwd->pw_class = class;

	if (pwd == NULL || (lc = login_getclass(pwd->pw_class)) == NULL)
		always_use_klogin = 1;
	else
		always_use_klogin = login_getcapbool(lc, "alwaysuseklogin", 0);

	/* Initialize krb5 context... */
	code = krb5_init_context(&context);

	/* Is there a valid krb v5 profile on this machine? */
	krb5_configed = (code == 0);

#if defined(PASSWD)
	if (wheel != NULL && strcmp(wheel, "yes") != 0 &&
    	    (!krb5_configed || pwd == NULL ||
	    k5oktologin(pwd->pw_name, instance,
	    strcmp(instance, "root") == 0 ? instance : pwd->pw_name))) {
		fprintf(back, BI_VALUE " errormsg %s\n",
		    auth_mkvalue("you are not in group wheel"));
		fprintf(back, BI_REJECT "\n");
		exit(1);
	}

	if (*instance == '\0' && pwd && *pwd->pw_passwd == '\0') {
		fprintf(back, BI_AUTH "\n");
		exit(0);
	}
#else
	if (!krb5_configed) {
		syslog(LOG_ERR, "Kerberos V5 not configured");
		exit(1);
	}
	if (pwd == NULL || k5oktologin(pwd->pw_name, instance, strcmp(instance, "root") == 0 ?
	    instance : pwd->pw_name)) {
		fprintf(back, BI_REJECT "\n");
syslog(LOG_ERR, "rejecting due to failed k5oktologin (%s, %s, %s)",
		pwd->pw_name, instance, strcmp(instance, "root") == 0 ?
			instance : pwd->pw_name);
		exit(1);
	}
#endif

	if (mode == 1) {
		fprintf(back, BI_SILENT "\n");
		exit(0);
	}

	(void)setpriority(PRIO_PROCESS, 0, -4);

	if (mode == 2) {
		mode = 0;
		c = -1;
		while (++c < sizeof(response) &&
		    read(3, &response[c], 1) == 1) {
			if (response[c] == '\0' && ++mode == 2)
				break;
			if (response[c] == '\0' && mode == 1)
				p = response + c + 1;
		}
		if (mode < 2) {
			syslog(LOG_ERR, "protocol error on back channel");
			exit(1);
		}
	} else
#if defined(PASSWD)
		p = getpass("Password:");
#else
		p = getpass("Kerberos V5 Password:");
#endif

	if (pwd) {
		rval = krb5_configed ? k5login(pwd, instance, localhost, p) : 1;
		if (rval == 0)
			if (*instance && strcmp(instance, "root") == 0)
				fprintf(back, BI_ROOTOKAY "\n");
			else
				fprintf(back, BI_AUTH "\n");
#if defined(PASSWD)
		else if (rval == 1) {
			if (wheel != NULL && strcmp(wheel, "yes") != 0)
				exit(1);
			if (*instance &&
			    (strcmp(instance, "root") != 0 ||
			    (pwd = getpwnam(instance)) == NULL)) {
				crypt(p, "xx");
				memset(p, 0, strlen(p));
				exit(1);
			}
			rval = strcmp(crypt(p, pwd->pw_passwd), pwd->pw_passwd);
			if (rval == 0)
				rval = login_check_expire(back, pwd, class,
				    lastchance);
		}
#endif
	}
#if defined(PASSWD)
	else
		crypt(p, "xx");
#endif
	memset(p, 0, strlen(p));

	if (!pwd || rval)
		exit(1);
	fprintf(back, BI_AUTH "\n");

	if (krb5ccname_env) {
		fprintf(back, BI_REMOVE " %s\n", krb5ccname_env);
		fprintf(back, BI_SETENV " KRB5CCNAME %s\n", krb5ccname_env);
#ifdef BI_UNSETENV
	} else {
		/* Make sure there isn't a lingering KRB5CCNAME from */
		/* a previous shell (happens when login_krb5 is running */
		/* as a result of an su) */
		fprintf(back, BI_UNSETENV " KRB5CCNAME\n");
#endif
	}
	return(0);
}

