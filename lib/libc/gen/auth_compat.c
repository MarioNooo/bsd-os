/*-
 * Copyright (c) 1995,1997 Berkeley Software Design, Inc. All rights reserved.
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
 *	This product includes software developed by Berkeley Software Design,
 *	Inc.
 * 4. The name of Berkeley Software Design, Inc.  may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI auth_compat.c,v 2.3 1999/09/08 04:10:40 prb Exp
 */
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <paths.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <login_cap.h>

struct rmfiles {
	struct rmfiles	*next;
	char		*file;
};

struct authopts {
	struct authopts	*next;
	char		*opt;
};

#define	MAXSPOOLSIZE	(8*1024)	/* Spool up to 8K of back info */

static char spool[MAXSPOOLSIZE];
static int spoolindex = 0;
static struct rmfiles *rmroot = NULL;
static struct authopts *optroot = NULL;

static void add_rmlist(char *);
static void spooldata(int);
static int _auth_script(char *, int, char *, va_list);

/*
 * Set up a known environment for all authentication scripts.
 */
static char *auth_environ[] = {
	"PATH=" _PATH_DEFPATH,
	"SHELL=" _PATH_BSHELL,
	NULL,
};

int
auth_check(char *fullname, char *class, char *style, char *service, int *stat)
{
	char path[MAXPATHLEN];
	int _stat;

	if (!stat)
		stat = &_stat;

	*stat = 0;

	if (!style)
		return (-1);
	if (!service)
		service = LOGIN_DEFSERVICE;

	snprintf(path, sizeof(path), _PATH_AUTHPROG "%s", style);
	if (auth_script(path, style, "-s", service, fullname, class, 0))
		return (*stat = 0);
	return ((*stat = auth_scan(0)) & AUTH_ALLOW);
}

int
auth_response(char *fullname, char *class, char *style, char *service,
    int *stat, char *challenge, char *response)
{
	char *data;
	char path[MAXPATHLEN];
	int _stat, datalen;

	if (!stat)
		stat = &_stat;

	*stat = 0;

	if (!style)
		return (-1);
	if (!service)
		service = LOGIN_DEFSERVICE;

	datalen = strlen(challenge) + strlen(response) + 2;

	if ((data = malloc(datalen)) == NULL) {
		syslog(LOG_ERR, "%m");
		warnx("internal resource failure");
		return (-1);
	}
	snprintf(data, datalen, "%s%c%s", challenge, 0, response);

	snprintf(path, sizeof(path), _PATH_AUTHPROG "%s", style);
	if (auth_script_data(data, datalen, path, style, "-s", service,
	    fullname, class, 0))
		*stat = 0;
	else
		*stat = auth_scan(0);
	free(data);

	return (*stat & AUTH_ALLOW);
}

void
auth_env()
{
	char *line;
	char *name;

    	for (line = spool; line < spool + spoolindex;) {
		if (!strncasecmp(line, BI_SETENV, sizeof(BI_SETENV)-1)) {
			line += sizeof(BI_SETENV) - 1;
			if (isblank(*line)) {
				for (name = line; isblank(*name); ++name)
					;
				for (line = name; *line && !isblank(*line);
				    ++line)
					;
				if (*line)
					*line++ = '\0';
				for (; isblank(*line); ++line)
					;
				if (*line != '\0' && setenv(name, line, 1))
					warn("setenv(%s, %s)", name, line);
			}
		} else
		if (!strncasecmp(line, BI_UNSETENV, sizeof(BI_UNSETENV)-1)) {
			line += sizeof(BI_UNSETENV) - 1;
			if (isblank(*line)) {
				for (name = line; isblank(*name); ++name)
					;
				for (line = name; *line && !isblank(*line);
				    ++line)
					;
				if (*line)
					*line++ = '\0';
				unsetenv(name);
			}
		}
		while (*line++)
			;
	}
}

char *
auth_value(char *what)
{
	char *line;
	char *name;
	char *value;
	int n;

    	for (line = spool; line < spool + spoolindex;) {
		if (!strncasecmp(line, BI_VALUE, sizeof(BI_VALUE)-1)) {
			line += sizeof(BI_VALUE) - 1;
			if (isblank(*line)) {
				for (name = line; isblank(*name); ++name)
					;
				for (line = name; !isblank(*line); ++line)
					;
				if (*line)
					*line++ = '\0';
				if (strcmp(name, what))
					continue;
				for (; isblank(*line); ++line)
					;
				value = strdup(line);
				if (value == NULL)
					return (NULL);

				/*
				 * XXX - There should be a more standardized
				 * routine for doing this sort of thing.
				 */
				for (line = name = value; *line; ++line) {
					if (*line == '\\') {
						switch (*++line) {
						case 'r':
							*name++ = '\r';
							break;
						case 'n':
							*name++ = '\n';
							break;
						case 't':
							*name++ = '\t';
							break;
						case '0': case '1': case '2':
						case '3': case '4': case '5':
						case '6': case '7':
							n = *line - '0';
							if (isdigit(line[1])) {
								++line;
								n <<= 3;
								n |= *line-'0';
							}
							if (isdigit(line[1])) {
								++line;
								n <<= 3;
								n |= *line-'0';
							}
							break;
						default:
							*name++ = *line;
							break;
						}
					} else
						*name++ = *line;
				}
				*name = '\0';
				return (value);
			}
		}
		while (*line++)
			;
	}
	return (NULL);
}

void
auth_rmfiles()
{
	while (rmroot) {
		unlink(rmroot->file);
		rmroot = rmroot->next;
	}
}

int
auth_script(char *path, ...)
{
	va_list ap;
	int r;

	va_start(ap, path);

	r = _auth_script(NULL, 0, path, ap);

	va_end(ap);

	return (r);
}

int
auth_script_data(char *data, int nbytes, char *path, ...)
{
	va_list ap;
	int r;

	va_start(ap, path);

	r = _auth_script(data, nbytes, path, ap);

	va_end(ap);

	return (r);
}

static int
_auth_script(char *data, int nbytes, char *path, va_list ap)
{
	struct authopts *e;
	pid_t pid;
	int status;
	int pfd[2];
	int argc;
	char *argv[64];		/* 64 args should more than enough */
#define	Nargc	(sizeof(argv)/sizeof(argv[0]))

	argc = 0;
	if ((argv[argc] = va_arg(ap, char *)) != NULL)
		++argc;

	for (e = optroot; e != NULL; e = e->next) {
		if (argc < Nargc - 2) {
			argv[argc++] = "-v";
			argv[argc++] = e->opt;
		} else {
			syslog(LOG_ERR, "too many authentication options");
			return (-1);
		}
	}
	while (argc < Nargc - 1 && (argv[argc] = va_arg(ap, char *)))
		++argc;

	if (argc >= Nargc - 1 && va_arg(ap, char *)) {
		syslog(LOG_ERR, "too many arguments");
		return (-1);
	}

	argv[argc] = NULL;

	spoolindex = 0;

	if (secure_path(path) < 0) {
		syslog(LOG_ERR, "%s: path not secure", path);
		warnx("invalid script: %s", path);
		return (-1);
	}

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, pfd) < 0) {
		syslog(LOG_ERR, "unable to create backchannel %m");
		warnx("internal resource failure");
		return (-1);
	}

	switch (pid = fork()) {
	case -1:
		close(pfd[0]);
		close(pfd[1]);
		syslog(LOG_ERR, "%s: %m", path);
		warnx("internal resource failure");
		return (-1);
	case 0:
#define	COMM_FD	3
		close(pfd[0]);
		if (pfd[1] != COMM_FD) {
			if (dup2(pfd[1], COMM_FD) < 0)
				err(1, "dup of backchannel");
			close(pfd[1]);
		}

		for (status = getdtablesize() - 1; status > COMM_FD; status--)
			close(status);

		execve(path, argv, auth_environ);
		syslog(LOG_ERR, "%s: %m", path);
		err(1, path);
	default:
		close(pfd[1]);
		if (data && nbytes)
			write(pfd[0], data, nbytes);
		spooldata(pfd[0]);
		close(pfd[0]);
		if (waitpid(pid, &status, 0) < 0) {
			syslog(LOG_ERR, "%s: waitpid: %m", path);
			warnx("internal failure");
			return (-1);
		}

		if (!WIFEXITED(status) || WEXITSTATUS(status))
			return (-1);
	}
	return (0);
}

int
auth_scan(int okay)
{
	char *line;

    	for (line = spool; line < spool + spoolindex;) {
		if (!strncasecmp(line, BI_REJECT, sizeof(BI_REJECT)-1)) {
			line += sizeof(BI_REJECT) - 1;
			if (!*line || *line == ' ' || *line == '\t') {
				while (*line == ' ' || *line == '\t')
					++line;
				if (!strcasecmp(line, "silent"))
					return (AUTH_SILENT);
				if (!strcasecmp(line, "challenge"))
					return (AUTH_CHALLENGE);
			}
			return (0);
		} else if (!strncasecmp(line, BI_AUTH, sizeof(BI_AUTH)-1)) {
			line += sizeof(BI_AUTH) - 1;
			if (!*line || *line == ' ' || *line == '\t') {
				while (*line == ' ' || *line == '\t')
					++line;
				if (*line == '\0')
					okay |= AUTH_OKAY;
				else if (!strcasecmp(line, "root"))
					okay |= AUTH_ROOTOKAY;
				else if (!strcasecmp(line, "secure"))
					okay |= AUTH_SECURE;
			}
		}
		else if (!strncasecmp(line, BI_REMOVE, sizeof(BI_REMOVE)-1)) {
			line += sizeof(BI_REMOVE) - 1;
			while (*line == ' ' || *line == '\t')
				++line;
			if (*line)
				add_rmlist(line);
		}
		while (*line++)
			;
	}

	return (okay);
}

int
auth_setopt(char *n, char *v)
{
	struct authopts *e;

	if ((e = malloc(sizeof(*e))) == NULL)
		return (-1);
	if ((e->opt = malloc(strlen(n)+strlen(v)+2)) == NULL) {
		free(e);
		return (-1);
	}
	sprintf(e->opt, "%s=%s", n, v);
	e->next = optroot;
	optroot = e;
	return(0);
}

void
auth_clropts()
{
	struct authopts *e;
	while ((e = optroot) != NULL) {
		optroot = optroot->next;
		free(e->opt);
		free(e);
	}
}

static void
spooldata(int fd)
{
	int r;
	char *b;

	while (spoolindex < sizeof(spool) - 1) {
		r = read(fd, spool + spoolindex, sizeof(spool)-spoolindex);
		if (r <= 0) {
			spool[spoolindex] = '\0';
			return;
		}
		b = spool + spoolindex;
		spoolindex += r;
		/*
		 * Go ahead and convert newlines into NULs to allow
		 * easy scanning of the file.
		 */
		while(r-- > 0)
			if (*b++ == '\n')
				b[-1] = '\0';
	}

	syslog(LOG_ERR, "Overflowed backchannel spool buffer");
	err(1, "System error in authentication program");
}

static void
add_rmlist(char *file)
{
	struct rmfiles *rm;

	if ((rm = malloc(sizeof(struct rmfiles))) == NULL) {
		syslog(LOG_ERR, "Failed to allocate rmfiles: %m");
		return;
	}
	if (!(file = strdup(file))) {
		syslog(LOG_ERR, "Failed to duplicate rmfile: %m");
		return;
	}
	rm->file = file;
	rm->next = rmroot;
	rmroot = rm;
}
