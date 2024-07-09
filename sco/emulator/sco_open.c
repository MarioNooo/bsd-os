/*
 * Copyright (c) 1993,1994,1995 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_open.c,v 2.3 1996/02/12 18:28:29 donn Exp
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "emulate.h"
#include "pathnames.h"
#include "sco_ops.h"
#include "sco.h"
#include "sco_sig_state.h"

/*
 * Code for translating filenames into file descriptors
 * and file ops.
 */

/*
 * Open() flags.
 *
 * Flag values come from the Intel iBCS2 document, p. 6-35.
 */

#define	SCO_O_NDELAY	0x0004
#define	SCO_O_SYNC	0x0010
#define	SCO_O_NONBLOCK	0x0080
#define	SCO_O_CREAT	0x0100
#define	SCO_O_TRUNC	0x0200
#define	SCO_O_EXCL	0x0400

const unsigned long open_in_bits[] = {
	SCO_O_NDELAY,	X_O_NDELAY,
	SCO_O_CREAT,	O_CREAT,
	SCO_O_TRUNC,	O_TRUNC,
	SCO_O_EXCL,	O_EXCL,
	SCO_O_NONBLOCK,	O_NONBLOCK,
	SCO_O_SYNC,	O_FSYNC,
	0
};
const unsigned long open_out_bits[] = {
	O_CREAT,	SCO_O_CREAT,
	O_TRUNC,	SCO_O_TRUNC,
	O_EXCL,		SCO_O_EXCL,
	O_NONBLOCK,	SCO_O_NONBLOCK,
	O_FSYNC,	SCO_O_SYNC,
	X_O_NDELAY,	SCO_O_NDELAY,
	0
};

#define	COMMON_OPEN_BITS	(O_ACCMODE|O_APPEND)
const struct xbits open_in_xbits = { COMMON_OPEN_BITS, open_in_bits };
const struct xbits open_out_xbits = { COMMON_OPEN_BITS, open_out_bits };

/* from iBCS2 p 6-35 */
const int fcntl_in_map[] = {
	F_DUPFD,
	F_GETFD,
	F_SETFD,
	F_GETFL,
	F_SETFL,
	F_GETLK,
	F_SETLK,
	F_SETLKW,
};
const int max_fcntl_in_map = sizeof (fcntl_in_map) / sizeof (int);

static struct filemap {
	char	*name;
	size_t	len;
	char	*replacement;
	size_t	rlen;
} *filemap;

static int checked_filemap;

char *curdir;

void
set_curdir(path)
	char *path;
{
	char *p;

	if (path && *path == '/') {
		if (curdir)
			sfree(curdir);
		if ((curdir = smalloc(strlen(path) + 2)) == 0)
			err(1, "set_curdir: %s", path);
		strcpy(curdir, path);
	} else {
		if (curdir == 0 || path == 0) {
			if (curdir)
				sfree(curdir);
			if ((curdir = smalloc(MAXPATHLEN + 2)) == 0)
				err(1, "set_curdir");
			if (getcwd(curdir, MAXPATHLEN) == 0)
				err(1, "set_curdir getcwd");
			strcat(curdir, "/");
		}
		if (path) {
			if ((p = smalloc(strlen(curdir) + strlen(path) + 3))
			    == 0)
				err(1, "set_curdir: %s/%s", curdir, path);
			strcpy(p, curdir);
			strcat(p, path);
			sfree(curdir);
			curdir = p;
		}
	}
	if (*curdir == '\0' || curdir[strlen(curdir) - 1] != '/')
		strcat(curdir, "/");
#ifdef DEBUG
	if (debug & DEBUG_FILEMAP)
		warnx("set_curdir: %s", curdir);
#endif
	return;
}

/*
 * Squeeze out '.' and '..' pathname components.
 */
static char *
squeeze_filename(s)
	char *s;
{
	char *src, *dst;

	for (src = dst = s;;) {
		for (; *src != '/'; *dst++ = *src++)
			if (*src == '\0') {
				*dst = '\0';
				while (--dst > s && *dst == '/')
					*dst = '\0';
				return (s);
			}
		if (src[1] != '.') {
			*dst++ = *src++;
			continue;
		}
		if (src[2] == '\0' || src[2] == '/') {
			src += 2;
			continue;
		}
		if (src[2] == '.' && (src[3] == '\0' || src[3] == '/')) {
			while (dst > s && *--dst != '/')
				;
			src += 3;
			continue;
		}
		*dst++ = *src++;
	}

	/* not reached */
	return (s);
}

int
sco_chdir(path)
	const char *path;
{
	int r;

	if ((r = chdir(path)) == -1)
		return (-1);
	set_curdir((char *)path);
	return (r);
}

/*
 * Read the file remapping table from a file.
 * XXX We're set up for linear search; if tables get long,
 * XXX we should think about a better search algorithm.
 */
static void
init_filemap()
{
	struct stat s;
	struct filemap *fp, *filemap_proto;
	char *buf, *p, *q, *c;
	ssize_t buflen;
	int f;
	int lines;

	checked_filemap = 1;

	if ((f = open(_PATH_FILEMAP, O_RDONLY)) == -1) {
#ifdef DEBUG
		if (debug & DEBUG_FILEMAP)
			warn("init_filemap: %s", _PATH_FILEMAP);
#endif
		return;
	}

	if (fstat(f, &s) == -1)
		err(1, "init_filemap: %s", _PATH_FILEMAP);
	if ((buf = smalloc(s.st_size + 1)) == 0)
		err(1, "init_filemap");
	if ((buflen = read(f, buf, s.st_size)) == -1)
		err(1, "init_filemap: %s", _PATH_FILEMAP);
	close(f);

	if (buflen == 0)
		return;

	for (p = buf, lines = 0; p < buf + buflen; ++p)
		if (*p == '\n') {
			*p = '\0';
			++lines;
		}
	if (*(p - 1) != '\0') {
		*p = '\0';
		++lines;
	}

	if ((filemap_proto = smalloc(sizeof *filemap * (lines + 1))) == 0)
		err(1, "init_filemap");

	for (p = buf, fp = filemap_proto; p < &buf[buflen]; p = q) {
		q = p + strlen(p) + 1;
		if (c = index(p, '#'))
			*c = '\0';
		if ((c = strtok(p, " \t")) == 0)
			continue;
		fp->name = c;
		fp->len = strlen(c);
		if ((c = strtok(0, " \t")) == 0) {
#ifdef DEBUG
			if (debug & DEBUG_FILEMAP)
				warnx("init_filemap: missing delimiter for %s",
					fp->name);
#endif
			fp->name = 0;
			continue;
		}
		fp->replacement = c;
		fp->rlen = strlen(c);
		++fp;
	}

	if (fp == filemap_proto) {
		sfree(filemap_proto);
		sfree(buf);
		return;
	}

	fp->name = 0;
	filemap = filemap_proto;
	return;
}

/*
 * Look for a path prefix match with an element in the file replacement table.
 * If we find one, replace the prefix and return the new string.
 */
static char *
lookup_filename(sco_file)
	char *sco_file;
{
	const struct filemap *fp;
	const char *p;
	char *file;
	size_t len;

	if (*sco_file != '/') {
		if (curdir == 0)
			set_curdir(0);
		if ((file = smalloc(strlen(curdir) + strlen(sco_file) + 1))
		    == 0)
			err(1, "lookup_filename");
		strcpy(file, curdir);
		strcat(file, sco_file);
		sco_file = squeeze_filename(file);
		if (*sco_file == '\0')
			strcpy(sco_file, "/");
	}
#ifdef DEBUG
	if (debug & DEBUG_FILEMAP)
		warnx("lookup_filename: %s", sco_file);
#endif
	len = strlen(sco_file);

	for (fp = filemap; fp->name; ++fp) {
		if (len < fp->len || bcmp(sco_file, fp->name, fp->len))
			continue;
		p = sco_file + fp->len;
		if (*p == '\0') {
			if ((file = smalloc(fp->rlen + 1)) == 0)
				err(1, "lookup_filename");
			bcopy(fp->replacement, file, fp->rlen + 1);
			return (file);
		}
		if (*p == '/') {
			len = len + 1 - fp->len;
			if ((file = smalloc(fp->rlen + len)) == 0)
				err(1, "lookup_filename");
			bcopy(fp->replacement, file, fp->rlen);
			bcopy(p, file + fp->rlen, len);
			return (file);
		}
	}

	return (0);
}

/*
 * Translate a SCO filename into a BSD filename.
 */
char *
filename_in(sco_file)
	const char *sco_file;
{
	char *p;

	if (!checked_filemap)
		init_filemap();

	if (filemap && (p = lookup_filename(sco_file)))
		return (p);

	return ((char *)sco_file);
}

/* from iBCS2 p 6-36 */
struct sco_flock {
	short	l_type;
	short	l_whence;
	long	l_start;
	long	l_len;
	short	l_sysid;
	short	l_pid;
};

/* from iBCS2 p 6-36: invert F_WRLCK and F_UNLCK */
#define	flocktype_in(x)		(x & 2 ? x ^ 1 : x)
#define	flocktype_out(x)	(x & 2 ? x ^ 1 : x)

void
flock_in(xsfl, fl)
	const struct flock *xsfl;
	struct flock *fl;
{
	const struct sco_flock *sfl = (const struct sco_flock *)xsfl;

	fl->l_type = flocktype_in(sfl->l_type);
	fl->l_whence = sfl->l_whence;	/* XXX identical */
	fl->l_start = sfl->l_start;
	fl->l_len = sfl->l_len;
	fl->l_pid = sfl->l_pid;
}

void
flock_out(fl, xsfl)
	const struct flock *fl;
	struct flock *xsfl;
{
	struct sco_flock *sfl = (struct sco_flock *)xsfl;

	sfl->l_type = flocktype_out(fl->l_type);
	if (fl->l_type == F_UNLCK) {
		sfl->l_whence = SEEK_SET;
		return;
	}
	sfl->l_whence = fl->l_whence;
	sfl->l_start = fl->l_start;
	sfl->l_len = fl->l_len;
	sfl->l_pid = fl->l_pid;
}

int max_fdtab;
struct fdbase **fdtab;
extern struct fdops regops;
extern struct fdops dirops;
extern struct fdops ttyops;
extern struct fdops sockops;

void
fd_register(f)
	int f;
{
	int oldmax;

	if (f >= max_fdtab) {
		oldmax = max_fdtab;
		if (max_fdtab == 0)
			max_fdtab = 32;
		while (max_fdtab <= f)
			max_fdtab *= 2;
		if ((fdtab = realloc(fdtab, max_fdtab * sizeof (*fdtab))) == 0)
			err(1, "file descriptor translation table");
		bzero(&fdtab[oldmax], (max_fdtab - oldmax) * sizeof (*fdtab));
	}

	if (fdtab[f])
		errx(1, "fd %d already open", f);
}

void
#ifdef __STDC__
fd_init(int f, mode_t mode, const char *filename, int flags)
#else
fd_init(f, mode, filename, flags)
	int f;
	mode_t mode;
	const char *filename;
	int flags;
#endif
{
	struct fdops *fdp;

	/* select ops based on mode and filename */
	/* XXX temporary */
	if (S_ISDIR(mode))
		fdp = &dirops;
	else if (S_ISCHR(mode) && isatty(f))
		fdp = &ttyops;
	else if ((mode & S_IFMT) == S_IFSOCK ||
	    strcmp(filename, _PATH_SOCKSYS) == 0)
		fdp = &sockops;
	else
		fdp = &regops;
	fdp->init(f, filename, flags);
}

/*
 * The generic open() syscall emulation.
 * The filename has been translated into a BSD filename
 * which may point at a replacement file.
 */
int
#ifdef __STDC__
sco_open(const char *filename, int flags, mode_t mode)
#else
sco_open(filename, flags, mode)
	const char *filename;
	int flags;
	mode_t mode;
#endif
{
	struct stat s;
	int f, xflags = flags;

	if (xflags & X_O_NDELAY)
		xflags = (xflags &~ X_O_NDELAY) | O_NONBLOCK;

	if ((f = commit_open(filename, xflags, mode)) == -1)
		return (-1);
	if (fstat(f, &s) == -1)
		err(1, "%s", filename);
	fd_init(f, s.st_mode, filename, flags);
	return (f);
}

int
#ifdef __STDC__
sco_creat(const char *filename, mode_t mode)
#else
sco_creat(filename, mode)
	const char *filename;
	mode_t mode;
#endif
{

	return (sco_open(filename, O_WRONLY|O_CREAT|O_TRUNC, mode));
}

int
sco_pipe()
{
	int fd[2];

	if (pipe(fd) == -1)
		return (-1);
	regops.init(fd[0], "", O_RDONLY);
	regops.init(fd[1], "", O_WRONLY);

	/* iBCS2 p 3-35 documents EDX hack */
	*program_edx = fd[1];
	return (fd[0]);
}

/*
 * Recognize file descriptors we inherited from our parent.
 * In the future, we may need to be more adept at recognizing special files.
 */
int
fd_inherit(f)
	int f;
{
	struct stat s;

	if (fstat(f, &s) == -1)
		return (-1);
	fd_init(f, s.st_mode, "", O_RDWR);
	return (0);
}
