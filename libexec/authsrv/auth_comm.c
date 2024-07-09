/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI auth_comm.c,v 1.3 2000/08/29 20:02:17 polk Exp
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <login_cap.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <vis.h>

#include "include.h"
#include "paths.h"

#define	AUTH_PORT	342

typedef struct {
	int	fd;
	char	input[256];
	char	*head;
	char	*tail;
	char	*mode;
	int	(*encrypt)(char *, int, void *);
	int	(*decrypt)(char *, int, void *);
	int	(*initiate)(int, void **);
	int	(*verify)(int, void **);
	void	*private;			/* algorithm specific data */
} auth_t;

int my_des_encrypt(char *, int, void *);
int my_des_decrypt(char *, int, void *);
int des_initiate(int, void **);
int des_verify(int, void **);

int ides_encrypt(char *, int, void *);
int ides_decrypt(char *, int, void *);
int ides_initiate(int, void **);
int ides_verify(int, void **);

int md5_encrypt(char *, int, void *);
int md5_decrypt(char *, int, void *);
int md5_initiate(int, void **);
int md5_verify(int, void **);

struct {
	char	*mode;
	int	(*encrypt)(char *, int, void *);
	int	(*decrypt)(char *, int, void *);
	int	(*initiate)(int, void **);
	int	(*verify)(int, void **);
} auth_modes[] = {
	{ "DES", my_des_encrypt, my_des_decrypt, des_initiate, des_verify, },
#if 0
	{ "IDES", ides_encrypt, ides_decrypt, ides_initiate, ides_verify, },
	{ "MD5", md5_encrypt, md5_decrypt, md5_initiate, md5_verify, },
#endif
};

int nauth_modes = (sizeof(auth_modes)/sizeof(auth_modes[0]));

static auth_t *find_auth(struct sockaddr_in);
static void random_init();

static int amserver = 0;
FILE	*trace = NULL;
int	traceall = 0;

int
fd_auth(void *v)
{
	return(((auth_t *)v)->fd);
}

void *
init_auth(char *user, char *class)		/* client side */
{
	auth_t *a;
	login_cap_t *lc;
	char *authserver;
	struct hostent *hp;
	struct sockaddr_in sin;
	int s, len;
	struct servent *sp;

	random_init();

	if (class == NULL) {
		struct passwd *t = getpwnam(user);
		if (!t)
			errx(1, "%s: no such user", user);
		class = t->pw_class;
	}
	if ((lc = login_getclass(class)) == NULL)
		errx(1, "%s: no such class", class);

	if ((authserver = login_getcapstr(lc, "authserver", 0, 0)) == NULL)
		errx(1, "no authentication server defined for this class");

	if ((hp = gethostbyname(authserver)) == 0)
		err(1, "%s", authserver);
	memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
	sin.sin_family = AF_INET;
	if ((sp = getservbyname("authsrv", "tcp")) == NULL) {
		warnx("authsrv/tcp: no such service -- defaulting to %d",
		    AUTH_PORT);
		sin.sin_port = htons(AUTH_PORT);
	} else
		sin.sin_port = sp->s_port;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "socket");

	if (connect(s, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0)
		err(1, "connect");

	a = find_auth(sin);

	for (len = 0; len < sizeof(a->input); ++len) {
		if (read(s, a->input + len, 1) != 1) {
			err(1, "reading encryption mode");
		}
		if (a->input[len] == '\0')
			break;
	}
	a->input[sizeof(a->input) - 1] = '\0';

	if (strcmp(a->mode, a->input) != 0)
		errx(1, "want mode %s, recieved mode %s", a->mode, a->input);

	if (a->initiate(s, &(a->private)) < 0) {
		free(a);
		close(s);
		return(NULL);
	}
	a->fd = s;
	return(a);
}

void *
start_auth(int s)
{
	struct sockaddr_in sin;
	int len;
	auth_t *a;

	random_init();

	amserver = 1;
	len = sizeof(sin);
	if (getpeername(s, (struct sockaddr *)&sin, &len) < 0) {
		syslog(LOG_ERR, "getpeername: %m");
		err(1, NULL);
	}

	a = find_auth(sin);
	write(s, a->mode, strlen(a->mode) + 1);

	if (a->verify(s, &(a->private)) < 0) {
		syslog(LOG_ERR, "unverified: %m");
		err(1, NULL);
	}
	return(a);
}

static char *
expandbuf(char *buf, int n)
{
	static char nbuf[24*1024];
	char *p = nbuf;

	while (n-- > 0) {
		*p++ = "0123456789ABCDEF"[(*buf >> 4) & 0xf];
		*p++ = "0123456789ABCDEF"[*buf++ & 0xf];
	}
	*p = 0;
	return(nbuf);
}

static char *
tracebuf(char *buf, int n)
{
	static char nbuf[24*1024];

	strvisx(nbuf, buf, n, VIS_CSTYLE|VIS_TAB|VIS_NL);
	return(nbuf);
}

int
read_auth(void *va, char *buf, int blen)
{
	auth_t *a = (auth_t *)va;
	char *p;
	int r;
	int retcode = -2;

again:
	if (a->head == a->tail)
		a->head = a->tail = a->input;
	else if ((p = strchr(a->head, '\n')) != NULL) {
		if (blen == 0)
			return(1);
		if ((r = p - a->head) < blen) {
			*p++ = '\0';
			memcpy(buf, a->head, r + 1);
			a->head = p;
			if (traceall && trace) {
				fprintf(trace, "read: %s\n", tracebuf(buf, r));
				fflush(trace);
			}
			return(r);
		}
		if (a->head != a->input) {
			memcpy(a->input, a->head, (a->tail - a->head) + 1);
			a->tail -= a->head - a->input;
			a->head = a->input;
		}
	}
	if (blen == 0) {
		fd_set rset;
		static struct timeval zero = { 0, 0 };
		FD_ZERO(&rset);
		FD_SET(a->fd, &rset);
		if ((r = select(a->fd+1, &rset, 0, 0, &zero)) != 1)
			return(0);
	}

	if (retcode != -2)
		return(retcode);

	r = read(a->fd, a->tail, (a->input + (sizeof(a->input) - 1) - a->tail));
	if (r > 0) {
		if (trace) {
			fprintf(trace, "READ: %s\n", expandbuf(a->tail, r));
			fflush(trace);
		}
		if ((r = a->decrypt(a->tail, r, a->private)) < 0) {
			if (amserver) {
				syslog(LOG_ERR, "corrupted data stream");
				exit(1);
			} else
				errx(1, "corrupted data stream");
		}
		a->tail += r;
		a->tail[0] = '\0';
		retcode = 0;
		goto again;
	}
	if (r < 0) {
		retcode = -1;
		goto again;
	}
	close(a->fd);
	return(0);
}

int
send_auth(void *va, char *buf)
{
	static char *pbuf = NULL;
	static int pbuflen = 0;
	auth_t *a = (auth_t *)va;
	int r, len;

	len = strlen(buf);
	if (len * 4 + 1 > pbuflen) {
		if (pbuf)
			free(pbuf);
		pbuflen = (len * 4 + 1024) & ~1023;
		pbuf = malloc(pbuflen);
		if (pbuf == NULL) {
			if (amserver) {
				syslog(LOG_ERR, "%m");
				exit(1);
			} else
				err(1, NULL);
		}
	}
	strcpy(pbuf, buf);
	buf = pbuf;

	if (traceall && trace) {
		fprintf(trace, "send: %s\n", tracebuf(buf, len));
		fflush(trace);
	}
	len = a->encrypt(buf, len, a->private);

	while (len > 0) {
		if ((r = write(a->fd, buf, len)) <= 0)
			return(-1);
		if (r > 0 && trace) {
			fprintf(trace, "SEND: %s\n", expandbuf(buf, r));
			fflush(trace);
		}
		len -= r;
		buf += r;
	}

	return(0);
}

static auth_t *
find_auth(struct sockaddr_in sin)
{
	struct stat sb;
	FILE *fp;
	char *data;
	char **addrs;
	struct hostent *hp;
	auth_t *a;
	char buf[MAXPATHLEN];
	int i;
	DIR *dirp;
	struct dirent *dp;


	if ((a = malloc(sizeof(auth_t))) == NULL) {
		if (amserver) {
			syslog(LOG_ERR, "calling malloc %m");
			exit (0);
		}
		err(1, NULL);
	}

	memset(a, 0, sizeof(auth_t));

	if ((dirp = opendir(_PATH_AUTHKEYS)) == NULL) {
		if (amserver) {
			syslog(LOG_ERR, "%s: %m", _PATH_AUTHKEYS);
			exit(0);
		}
		err(1, _PATH_AUTHKEYS);
	}

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;
		if ((hp = gethostbyname(dp->d_name)) == 0)
			continue;
		
		for (addrs = hp->h_addr_list; *addrs; ++addrs)
			if (memcmp(&sin.sin_addr, *addrs, hp->h_length) == 0)
				break;
		if (*addrs == NULL)
			continue;
		closedir(dirp);
		snprintf(buf, sizeof(buf), "%s/%s", _PATH_AUTHKEYS, dp->d_name);

		if (secure_path(buf) ||
		    stat(buf, &sb) < 0 ||
		    (sb.st_mode & (S_IRGRP|S_IROTH)) != 0) {
			if (amserver) {
				syslog(LOG_ERR, "%s: not secure", buf);
				exit(0);
			}
			errx(1, "%s: not secure", buf);
		}
		if ((fp = fopen(buf, "r")) == 0) {
			if (amserver) {
				syslog(LOG_ERR, "%s: %m", buf);
				exit(0);
			}
			err(1, buf);
		}
		if (fgets(buf, sizeof(buf), fp) == NULL)
			errx(1, "%s: empty file", dp->d_name);

		if ((data = strchr(buf, ' ')) != NULL) {
			*data++ = 0;
			if ((data = strdup(data)) == NULL) {
				if (amserver) {
					syslog(LOG_ERR, "%m");
					exit (0);
				}
				err(1, NULL);
			}
		}
		for (i = 0; i < nauth_modes; ++i)
			if (strcmp(auth_modes[i].mode, buf) == 0)
				break;
		if (i >= nauth_modes) {
			if (amserver) {
				syslog(LOG_ERR, "authclient has unknown authentication mode");
				exit (0);
			}
			errx(1, "authserver has unknown authentication mode");
		}
		a->encrypt = auth_modes[i].encrypt;
		a->decrypt = auth_modes[i].decrypt;
		a->initiate = auth_modes[i].initiate;
		a->verify = auth_modes[i].verify;
		a->mode = auth_modes[i].mode;
		a->private = data;
		return(a);
	}
	closedir(dirp);
	if (amserver) {
		syslog(LOG_ERR, "authclient %s not registered",
		    inet_ntoa(sin.sin_addr));
		exit (0);
	}
	errx(1, "authserver [%s] not registered", inet_ntoa(sin.sin_addr));
}

u_long
random_auth()
{
	return(random());
}

static void
random_init()
{
	static int done = 0;

	if (done)
		return;
	srandom(getpid() ^ time(0));		/* XXX - make this better */
	done = 1;
}
