/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI stdio_compat.c,v 2.3 1997/06/23 20:51:37 donn Exp
 */

/*
 * Compatibility hacks for C Standard I/O in the emulated SCO shared C library.
 * This code is NOT general and is present solely to support the use
 * of BSD library objects under the SCO emulator.
 *
 * XXX If the C library changes, we can get screwed...
 * XXX Worse, this code knows about BSD standard I/O innards.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shlib.h"

/* from iBCS2 p 6-54 */

#define	SCO_BUFSIZ	1024

#define	SPRINTF_FILENO	60

#define	SCO_IOFBF	0
#define	SCO_IONBF	0x04
#define	SCO_IOEOF	0x10
#define	SCO_IOERR	0x20
#define	SCO_IOLBF	0x40

/* by observation */

#define	SCO_RD		0x01
#define	SCO_WR		0x02
#define	SCO_MBF		0x08
#define	SCO_RW		0x80

/* from iBCS2 p 6-55 */

typedef struct {
	int		_cnt;
	unsigned char	*_ptr;
	unsigned char	*_base;
	char		_flag;
	char		_file;
} sco_FILE;

/* a guess, based on where it's used */

extern sco_FILE *_libc__iob;
extern sco_FILE *_libc__lastbuf;
extern unsigned char **_libc__bufendtab;

/* from iBCS2 p 6-54 */

#define	sco_stdin	(&_libc__iob[0])
#define	sco_stdout	(&_libc__iob[1])
#define	sco_stderr	(&_libc__iob[2])

static FILE **sf_to_bf;
static char *sf_flag_cache;

__dead void
panic(const char *s)
{

	sco_write(STDERR_FILENO, "libc_s panic: ", 14);
	sco_write(STDERR_FILENO, s, strlen(s));
	sco_write(STDERR_FILENO, "\n", 1);
	sco__exit(1);
}

int sco_nfile;

void
set_sco_nfile(void)
{
	struct rlimit rl;

	sco_nfile = _libc__lastbuf - _libc__iob;

	if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
		return;
	if (sco_nfile > rl.rlim_cur) {
		rl.rlim_cur = sco_nfile;
		setrlimit(RLIMIT_NOFILE, &rl);
	}
}

static void
set_sf_to_bf(void)
{

	if (sco_nfile == 0)
		set_sco_nfile();

	sf_to_bf = (FILE **)malloc(sco_nfile * sizeof (FILE *));
	sf_flag_cache = (char *)malloc(sco_nfile);
	if (sf_to_bf == 0 || sf_flag_cache == 0)
		panic("FILE_in malloc");
	memset(sf_to_bf, sco_nfile * sizeof (FILE *), '\0');
	memset(sf_flag_cache, sco_nfile, '\0');
	sf_to_bf[0] = stdin;
	sf_flag_cache[0] = sco_stdin->_flag;
	sf_to_bf[1] = stdout;
	sf_flag_cache[1] = sco_stdout->_flag;
	sf_to_bf[2] = stderr;
	sf_flag_cache[2] = sco_stderr->_flag;

	/* special initialization hack */
	stderr->_bf._base = sco_stderr->_base;
	stderr->_bf._size = _libc__bufendtab[2] - sco_stderr->_base;
	stderr->_p = sco_stderr->_ptr;
}

static int
eofread(void *cookie, char *buf, int len)
{

	return (0);
}

/*
 * Check to see whether a macro or an unshared stdio routine
 * has modified the pointers in the SCO stdio structure
 * since the last time we saw it.
 * If so, update the corresponding BSD stdio structure.
 */
FILE *
FILE_in(FILE *bf, sco_FILE *sf)
{
	int did_fdopen = 0;
	int f = sf->_file;
	int flags, mode;
	char *type;
	size_t len;

	if (sf_to_bf == 0)
		set_sf_to_bf();

	/* handle sscanf() */
	if (f == SPRINTF_FILENO) {
		/* bf points to a stack FILE; re-entrancy defense */
		bf->_flags = __SRD;
		bf->_read = eofread;
		bf->_bf._base = bf->_p = sf->_base;
		bf->_bf._size = bf->_r = sf->_cnt;
		bf->_ub._base = NULL;
		bf->_lb._base = NULL;
		return (bf);
	}

	bf = sf_to_bf[f];

	/* handle fdopen() */
	if (bf == NULL) {
		/*
		 * Our worst case: someone fdopen()ed a descriptor and
		 * putc'ed a buffer's worth of data to it.
		 * We assume that the setvbuf() and putc() adjustments
		 * below will sync us up with the SCO stdio buffer.
		 */
		if ((flags = sco_fcntl(f, F_GETFL, 0)) == -1)
			panic("FILE_in fcntl");

		switch (flags & (O_ACCMODE|O_APPEND)) {
		case O_RDONLY:
		default:
			type = "r";
			break;
		case O_WRONLY:
			type = "w";
			break;
		case O_RDWR:
			type = "r+";
			break;
		case O_WRONLY|O_APPEND:
			type = "a";
			break;
		case O_RDWR|O_APPEND:
			type = "a+";
			break;
		}

		bf = fdopen(f, type);
		did_fdopen = 1;
		bf->_flags |= __SNPT;	/* rewind() is unshared */
		sf_to_bf[f] = bf;
	}

	/* handle clearerr() */
	if (sf->_flag != sf_flag_cache[f] &&
	    (sf->_flag & (SCO_IOEOF|SCO_IOERR)) == 0) {
		sf_flag_cache[f] &= ~(SCO_IOEOF|SCO_IOERR);
		clearerr(bf);
	}

	/* handle setvbuf() */
	len = _libc__bufendtab[sf->_file] - sf->_base;
	if (sf->_base &&
	    ((sf->_flag ^ sf_flag_cache[f]) & (SCO_IONBF|SCO_IOLBF) ||
	    bf->_bf._base != sf->_base || bf->_bf._size != len)) {
		if (bf->_bf._base != sf->_base)
			/* don't try to free current buffer */
			bf->_flags &= ~__SMBF;
		mode = _IOFBF;
		if (sf->_flag & SCO_IONBF)
			mode = _IONBF;
		else if (sf->_flag & SCO_IOLBF)
			mode = _IOLBF;
		setvbuf(bf, (char *)(did_fdopen ? 0 : sf->_base), mode, len);
		sf_flag_cache[f] = sf->_flag;
	}

	/* handle getc()/putc() */
	if (bf->_p != sf->_ptr ||
	    (sf->_flag & SCO_IOLBF) == 0 && sf->_cnt < 0) {
		bf->_p = sf->_ptr;
		bf->_r = bf->_w = sf->_cnt;
	}

	return (bf);
}

/*
 * Update a SCO FILE structure after a BSD stdio operation.
 * Fopen() needs to do a little more work.
 */
void
FILE_out(FILE *bf, sco_FILE *sf)
{

	if (bf->_flags & __SRD)
		sf->_cnt = bf->_r;
	else
		sf->_cnt = bf->_w;
	sf->_ptr = bf->_p;
	if (sf->_base == 0 && bf->_bf._base) {
		sf->_base = bf->_bf._base;
		_libc__bufendtab[sf->_file] = bf->_bf._base + bf->_bf._size;
		if (bf->_flags & __SMBF)
			sf->_flag |= SCO_MBF;	/* for unshared setvbuf() */
		if (bf->_flags & __SLBF)
			sf->_flag |= SCO_IOLBF;
	}
	if (feof(bf))
		sf->_flag |= SCO_IOEOF;
	if (ferror(bf))
		sf->_flag |= SCO_IOERR;
	sf_flag_cache[sf->_file] = sf->_flag;
}

/*
 * Emulations of SCO stdio routines.
 */

static int
sco_clean(FILE *bf)
{

	sco_fclose(&_libc__iob[fileno(bf)]);
}

void
sco__cleanup(void)
{

	_fwalk(sco_clean);
}

sco_FILE *
sco__findiop(void)
{
	sco_FILE *f;

	/* a best guess at how it works */
	for (f = _libc__iob; f < _libc__lastbuf; ++f)
		if (f->_flag == 0) {
			memset(f, sizeof *f, '\0');
			return (f);
		}
	return (0);
}

int
sco_fclose(sco_FILE *sf)
{
	FILE *bf = FILE_in(0, sf);

	sf_to_bf[sf->_file] = 0;
	sf->_flag = 0;

	return (fclose(bf));
}

int
sco__flsbuf(int c, sco_FILE *sf)
{
	FILE *bf = FILE_in(0, sf);
	int n;

	n = __sputc(c, bf);
	FILE_out(bf, sf);
	return (n);
}

sco_FILE *
sco_fopen(const char *path, const char *mode)
{
	sco_FILE *sf = sco__findiop();
	FILE *bf = fopen(path, mode);

	if (bf == 0)
		return (0);

	/*
	 * The SCO fopen(3) man page actually says:  'If the value
	 * of the file descriptor passed to fdopen = 60, then fdopen
	 * will fail and return an error.' (!!!)
	 */
	if (fileno(bf) == SPRINTF_FILENO) {
		if ((bf->_file = dup(SPRINTF_FILENO)) == -1)
			return (0);
		close(SPRINTF_FILENO);
	}

	if (sf_to_bf == 0)
		set_sf_to_bf();
	sf_to_bf[fileno(bf)] = bf;
	setvbuf(bf, 0, _IOFBF, SCO_BUFSIZ);
	bf->_flags &= ~__SOPT;
	bf->_flags |= __SNPT;
	sf->_file = fileno(bf);
	if (mode[0] == 'r')
		if (mode[1] == '+')
			sf->_flag |= SCO_RW;
		else
			sf->_flag |= SCO_RD;
	else
		sf->_flag |= SCO_WR;
	if (bf->_flags & __SMBF)
		sf->_flag |= SCO_MBF;
	FILE_out(bf, sf);

	return (sf);
}

int
sco_setbuf(sco_FILE *sf, char *buf)
{
	FILE *bf = FILE_in(0, sf);
	int n = setvbuf(bf, buf, _IOFBF, SCO_BUFSIZ);

	sf->_base = 0;
	FILE_out(bf, sf);
	return (n);
}

#define	BODY(type, call, sf) \
{ \
	FILE *bf = FILE_in(0, sf); \
	type n = call; \
\
	FILE_out(bf, sf); \
	return (n); \
}

#define	TBODY(call, sf) \
{ \
	FILE temp; \
	FILE *bf = FILE_in(&temp, sf); \
	int n = call; \
\
	FILE_out(bf, sf); \
	return (n); \
}

#define	VBODY(call, sf) \
{ \
	FILE *bf = FILE_in(0, sf); \
	int n; \
	va_list ap; \
\
	va_start(ap, format); \
	n = call; \
	va_end(ap); \
\
	FILE_out(bf, sf); \
	return (n); \
}

#define	PBODY(s) \
{ \
\
	panic(s " unimplemented"); \
}

int sco__doprnt(char *format, va_list ap, sco_FILE *sf)
	BODY(int, vfprintf(bf, format, ap), sf)
int sco__bufsync() PBODY("_bufsync")
int sco__filbuf(sco_FILE *sf) TBODY(__srget(bf), sf)
int sco__findbuf() PBODY("_findbuf")
int sco__wrtchk() PBODY("_wrtchk")
int sco__xflsbuf() PBODY("_xflsbuf");
int sco_fflush(sco_FILE *sf) BODY(int, fflush(bf), sf)
int sco_fgetc(sco_FILE *sf) BODY(int, fgetc(bf), sf)
char *sco_fgets(char *s, size_t len, sco_FILE *sf)
	BODY(char *, fgets(s, len, bf), sf)
int sco_fprintf(sco_FILE *sf, const char *format, ...)
	VBODY(vfprintf(bf, format, ap), sf)
int sco_fputc(int c, sco_FILE *sf) BODY(int, fputc(c, bf), sf)
int sco_fputs(const char *s, sco_FILE *sf) BODY(int, fputs(s, bf), sf)
int sco_fread(void *p, size_t len, size_t m, sco_FILE *sf)
	BODY(int, fread(p, len, m, bf), sf)
FILE *sco_freopen(const char *path, const char *mode, sco_FILE *sf)
	BODY(FILE *, freopen(path, mode, bf), sf)
int sco_fseek(sco_FILE *sf, long o, int w) BODY(int, fseek(bf, o, w), sf)
int sco_fwrite(const void *p, size_t len, size_t m, sco_FILE *sf)
	BODY(int, fwrite(p, len, m, bf), sf)
int sco_getchar(void) BODY(int, getchar(), sco_stdin)
char *sco_gets(char *s) BODY(char *, gets(s), sco_stdin)
int sco_getw(sco_FILE *sf) BODY(int, getw(bf), sf)
int sco_printf(const char *format, ...) VBODY(vprintf(format, ap), sco_stdout)
int sco_putchar(int c) BODY(int, putchar(c), sco_stdout)
int sco_puts(const char *s) BODY(int, puts(s), sco_stdout)
int sco_putw(int w, sco_FILE *sf) BODY(int, putw(w, bf), sf)
int sco_ungetc(int c, sco_FILE *sf) TBODY(ungetc(c, bf), sf)
