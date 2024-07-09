/*-
 * Copyright (c) 1993, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI util.c,v 2.6 1995/11/30 21:08:31 bostic Exp
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "installsw.h"
#include "pathnames.h"

static int	remote_command __P((char *, char *));
static int	tape_seek __P((char *));

/*
 * get_remote_file --
 *	Retrieve a file from a remote host, and store it into a local file.
 */
int
get_remote_file(rname, lname)
	char *rname, *lname;
{
	struct stat sb;
	char locpart[1024], rempart[1024];

	/* XXX: Use the most portable pathnames. */
	(void)snprintf(rempart, sizeof(rempart), "/bin/cat %s/%s",
	    device, hide_meta(map_name(rname)));
	(void)snprintf(locpart, sizeof(locpart), "> %s ", lname);

	return (remote_local_command(rempart, locpart, 0) ||
	    stat(lname, &sb) || sb.st_size == 0);
}

/*
 * build_extract_command --
 *	Build an extraction command, optionally for a remote host.
 *
 * Format is for local commands is:
 *	(dd | compress) 2>tmp_file1 | pax
 *
 * Format for remote commands is:
 * Remote:
 *	dd
 * Local:
 *	| compress | pax
 *
 * This magic is so we can play with remote_local_command.
 */
char *
build_extract_command(pkg, pkgname, rpp, silent)
	PKG *pkg;
	char *pkgname, **rpp;
	int silent;
{
	static char locpart[MAXPATHLEN], rempart[MAXPATHLEN];
	char *p;

	if (location == L_REMOTE) {
		(void)snprintf(rempart, sizeof(rempart),
		    "%s bs=10k if=%s", _PATH_DD, hide_meta(map_name(pkgname)));
		p = locpart;
	} else {
		rempart[0] = '\0';
		p = locpart;
		p += snprintf(p, sizeof(locpart),
		    "(%s bs=10k if=%s", _PATH_DD, hide_meta(pkgname));
	}
	if (pkg->type == T_COMPRESSED)
		p += sprintf(p, "| %s", _PATH_UNCOMPRESS);
	if (pkg->type == T_GZIPPED)
		p += sprintf(p, "| %s -q", _PATH_UNZIP);

	if (location == L_LOCAL)
		p += sprintf(p, ") 2>%s", STATUS);

	p += sprintf(p, "| %s %s%s", _PATH_PAX, PAX_ARGS,
	    silent ? " > /dev/null 2>1" : "");

#ifdef DEBUG
	(void)fprintf(stderr,
	    "\n\textract command: rem: %s loc: %s\n", rempart, locpart);
#endif
	if (rpp != NULL)
		*rpp = rempart;
	return (locpart);
}

/* 
 * remote_local_command:
 *
 * Execute a command (with status) where part is executed locally and part
 * is executed remotely, e.g.
 *
 *	remote: "command", local: "> local_file"
 *
 * results in the execution of: 
 *
 * rsh -n [-l user] host
 *    "/bin/sh -c 'command 2>tmp_file1; echo $? >>tmp_file1'" > local_file
 *
 * This means that tmp_file1 has the status of the remote commands.
 *
 * NB:
 * We can't separate out the error output of the local commands, because
 * we're parsing the output of pax -v, which appears on stderr.
 */
int
remote_local_command(remote, local, silent)
	char *remote, *local;
	int silent;
{
	FILE *fp;
	int rval;
	char *p, buf[1024], cmd[1024];

	p = cmd;

	/* XXX: Don't use a path; hope that the user has it in theirs. */
	p += sprintf(p, "rsh -n");
	if (remote_user != NULL)
		p += sprintf(p, " -l %s", remote_user);
	p += sprintf(p, " %s", remote_host);

	/* XXX: Use the most portable pathnames. */
	(void)sprintf(p, " \"/bin/sh -c '%s 2>%s; echo $? >>%s'\" %s",
	    remote, STATUS, STATUS, local);
#ifdef DEBUG
	(void)printf("remote command: %s\n", cmd);
	fflush(stdout);
#endif
	/* Execute the command. */
	rval = system(cmd);
	rval = rval == -1 || rval == 127 ||
	    !WIFEXITED(rval) || WEXITSTATUS(rval) ? 1 : 0;

	/*
	 * Read and delete the remote status file.  The last line of the
	 * remote status file has the exit value of the remote process.
	 *
	 * XXX: Use the most portable pathnames.
	 */
	(void)sprintf(p, " \"/bin/cat %s; /bin/rm -f %s\"", STATUS, STATUS);

#ifdef DEBUG
	(void)printf("remote status: %s\n", cmd);
	fflush(stdout);
#endif
	if ((fp = popen(cmd, "r")) == NULL) {
		warn("popen: %s", cmd);
		rval = 1;
	} else {
		cmd[0] = '\0';
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			if (!silent && cmd[0] != '\0')
				(void)printf("%s", cmd);
			(void)strcpy(cmd, buf);
		}
		if (!isdigit(cmd[0]) || atoi(cmd) != 0)
			rval = 1;
		(void)fflush(stdout);
	}
	if (pclose(fp))
		rval = 1;
	return (rval);
}

/*
 * hide_meta --
 *	Escape the shell meta-characters in the string with backslashes.
 *	Currently only does `;' but it's easy to extend.
 */
char *
hide_meta(name)
	char *name;
{
	char *p, *q, buf[MAXPATHLEN * 2];

	if ((p = strchr(name, ';')) == NULL)
		return (name);

	for (q = buf, p = name;;) {
		if (*p == ';')
			*q++ = '\\';
		if ((*q++ = *p++) == '\0')
			break;
	}
	if (q - buf > MAXPATHLEN)
		err(1, "%s: meta character escaped pathname too long", name);
	memmove(name, buf, q - buf);
	return (name);
}

/*
 * mkdirp --
 *	Build a directory path.
 */
void
mkdirp(path)
	char *path;
{
	pid_t pid;
	int status;

	/*
	 * Ignore any error messages or exit status, and don't let mkdir
	 * try and talk to the user.  If mkdir fails, we'll find out when
	 * we do the chdir.
	 */
	if ((pid = vfork()) == 0) {
		(void)close(0);
		(void)close(1);
		(void)close(2);
		(void)setsid();
		execl(_PATH_MKDIR, "mkdir", "-p", path, NULL);
	}
	(void)waitpid(pid, &status, 0);
}

/*
 * tape_setup --
 *	Move to a specific file on the tape.
 */
int
tape_setup(fileno, fatal)
	int fileno, fatal;
{
	char cmd[MAXPATHLEN];
	
	if (fileno == 0 || fileno < tfileno) {
		(void)printf("\tRewinding tape...\n");
		(void)snprintf(cmd,
		    sizeof(cmd), "%s -f %s rew", _PATH_MT, device);
		if (tape_seek(cmd))
			goto err;
		tfileno = 0;
	}
	if (fileno > tfileno) {
		(void)printf("\tFast forwarding tape to file %d...\n",
		    fileno);
		(void)snprintf(cmd, sizeof(cmd),
		    "%s -f %s fsf %d", _PATH_MT, device, fileno - tfileno);
		if (tape_seek(cmd))
			goto err;
		tfileno = fileno;
	}
	return (0);

err:	if (fatal)
		exit (1);
	tfileno = 99999;			/* Force a rewind. */
	return (1);
}

/*
 * tape_seek --
 *	Move to a specific file on the tape.
 */
static int
tape_seek(cmd)
	char *cmd;
{
	int rval;

	if (location == L_REMOTE) {
		if (remote_command(cmd, remote_host)) {
			(void)printf("\t%s: %s: tape error\n",
			    remote_host, cmd);
			return (1);
		}
	} else {
		rval = system(cmd);
		if (rval == -1 || rval == 127 ||
		    !WIFEXITED(rval) || WEXITSTATUS(rval)) {
			(void)printf("\t%s: tape error\n", cmd);
			return (1);
		}
	}

	(void)sleep(TAPESLEEP);
	(void)printf("\tsucceeded.\n");
	return (0);
}

/*
 * tape_rew --
 *	Rewind the tape.
 */
void
tape_rew()
{
	if (media == M_TAPE && tfileno != 0) {
		(void)printf("\n");
		(void)tape_setup(0, 1);
	}
}

/*
 * remote_command --
 *	Execute a remote command and return its status.
 */
static int
remote_command(cmd, host)
	char *cmd, *host;
{
	FILE *fp;
	int  status;
	char *p, buf[MAXPATHLEN + 128];

	p = buf;
	/* Don't use a path; hope that the user has it in theirs. */
	p += sprintf(p, "rsh -n");
	if (remote_user != NULL)
		p += sprintf(p, "-l %s ", remote_user);
	p += sprintf(p, "%s ", host);

	/* XXX: Use the most portable pathnames. */
	p += sprintf(p, "\"/bin/sh -c '%s ; echo REMOTE_STATUS: \\$?'\"", cmd);

#ifdef DEBUG
	(void)printf("\n*** remote_command: %s\n", buf);
#endif
	if ((fp = popen(buf, "r")) == NULL)
		return (1);

	/*
	 * XXX
	 * Why is setlinebuf necessary?
	 */
	(void)setlinebuf(fp);
	for (status = 1; fgets(buf, BUFSIZ, fp) != NULL;)
		if (!strncmp(buf, "REMOTE_STATUS:", 14))
			status = atoi(&buf[15]);
		else
			(void)printf(buf);
	return (pclose(fp) < 0 ? 1 : status);
}

/*
 * log --
 *	Log the action.
 */
void
log(pkg, action)
	PKG *pkg;
	char *action;
{
	time_t t;
	int fd, len;
	char buf[MAXPATHLEN];

	(void)snprintf(buf, sizeof(buf), "%s/%s", rootdir, _PATH_LOG);
	if ((fd = open(buf, O_WRONLY | O_APPEND | O_CREAT,
	    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0)
		return;

	(void)time(&t);
	len = snprintf(buf, sizeof(buf),
	    "package %s %s %s", pkg->name, action, ctime(&t));
	(void)write(fd, buf, len);
	(void)close(fd);
}

static char *argv[30];
static char **cargvp;

/*
 * clean_add --
 *	Add a temporary file to the list to be deleted.
 */
void
clean_add(name)
	char *name;
{
	if (cargvp == NULL) {
		cargvp = argv;
		*cargvp++ = "rm";
		*cargvp++ = "-rf";
	}
	if ((*cargvp = strdup(name)) != NULL)
		++cargvp;
}

/*
 * clean_up --
 *	Delete any the temporary files we may have created.
 */
void
clean_up()
{
	pid_t pid;
	int status;

	if (cargvp == NULL)
		return;

	/*
	 * Ignore any error messages or exit status, and don't let rm
	 * try and talk to the user.
	 */
	if ((pid = vfork()) == 0) {
		(void)close(0);
		(void)close(1);
		(void)close(2);
		(void)setsid();
		execv(_PATH_RM, argv);
	}
	(void)waitpid(pid, &status, 0);
}
