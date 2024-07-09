/*	BSDI stdlib.h,v 2.13 2003/06/02 20:48:38 torek Exp	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)stdlib.h	8.5 (Berkeley) 5/19/95
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_WCHAR_T_
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
typedef	_BSD_WCHAR_T_	rune_t;
#endif
typedef	_BSD_WCHAR_T_	wchar_t;
#undef	_BSD_WCHAR_T_
#endif

typedef struct {
	int quot;		/* quotient */
	int rem;		/* remainder */
} div_t;

typedef struct {
	long quot;		/* quotient */
	long rem;		/* remainder */
} ldiv_t;

#ifndef NULL
#define	NULL	0
#endif

#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0

#define	RAND_MAX	32767

extern int __mb_cur_max;
#define	MB_CUR_MAX	__mb_cur_max

#include <sys/cdefs.h>

__BEGIN_DECLS
__dead	 void abort(void) __attribute__((__volatile__));
__pure	 int abs(int) __attribute__((__const__));
int	 atexit(void (*)(void));
double	 atof(const char *);
int	 atoi(const char *);
long	 atol(const char *);
void	*bsearch(const void *, const void *, size_t,
	    size_t, int (*)(const void *, const void *));
void	*calloc(size_t, size_t);
__pure	 div_t div(int, int) __attribute__((__const__));
__dead	 void exit(int) __attribute__((__volatile__));
void	 free(void *);
char	*getenv(const char *);
__pure	 long labs(long) __attribute__((__const__));
__pure	 ldiv_t ldiv(long, long) __attribute__((__const__));
void	*malloc(size_t);
void	 qsort(void *, size_t, size_t,
	    int (*)(const void *, const void *));
int	 rand(void);
int	 rand_r(unsigned int *);
void	*realloc(void *, size_t);
void	 srand(unsigned);
double	 strtod(const char *, char **);
long	 strtol(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
int	 system(const char *);

/* This is in C99 but not C89. */
long double strtold(const char *, char **);

/* These functions are from C99, which does have 'long long'.  */
__extension__ long long strtoll(const char *, char **, int);
__extension__ unsigned long long strtoull(const char *, char **, int);

/* These are currently just stubs. */
int	 mblen(const char *, size_t);
size_t	 mbstowcs(wchar_t *, const char *, size_t);
int	 wctomb(char *, wchar_t);
int	 mbtowc(wchar_t *, const char *, size_t);
size_t	 wcstombs(char *, const wchar_t *, size_t);

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)

int	 putenv(const char *);
int	 setenv(const char *, const char *, int);

#ifndef alloca
void	*alloca(size_t);		/* built-in for gcc */
#endif
char	*getbsize(int *, long *);

					/* getcap(3) functions */
char	*cgetcap(char *, char *, int);
int	 cgetclose(void);
int	 cgetent(char **, char **, char *);
int	 cgetfirst(char **, char **);
int	 cgetmatch(char *, char *);
int	 cgetnext(char **, char **);
int	 cgetnum(char *, char *, long *);
int	 cgetset(char *);
int	 cgetstr(char *, char *, char **);
int	 cgetustr(char *, char *, char **);

double	drand48(void);		/* rand48(3) functions */
double	erand48(unsigned short *);
long	jrand48(unsigned short *);
void	lcong48(unsigned short *);
long	lrand48(void);
long	mrand48(void);
long	nrand48(unsigned short *);
unsigned short	*seed48(unsigned short *);
void	srand48(long);

int	 daemon(int, int);
char	*devname(int, int);
int	 getloadavg(double [], int);

/* POSIX.2 puts getopt() in unistd.h; this is for compatibility */
extern char *optarg;			/* getopt(3) external variables */
extern int opterr, optind, optopt, optreset;
int	 getopt(int, char * const *, const char *);

extern char *suboptarg;			/* getsubopt(3) external variable */
int	 getsubopt(char **, char * const *, char **);

int	 heapsort(void *, size_t, size_t,
	    int (*)(const void *, const void *));
char	*initstate(unsigned int, char *, int);
int	 mergesort(void *, size_t, size_t,
	    int (*)(const void *, const void *));
int	 radixsort(const unsigned char **, int, const unsigned char *,
	    unsigned);
int	 sradixsort(const unsigned char **, int, const unsigned char *,
	    unsigned);
long	 random(void);
char	*realpath(const char *, char *);
char	*setstate(char *);
void	 srandom(unsigned int);
/* These interfaces are deprecated in favor of strtoll() and strtoull().  */
__extension__ long long strtoq(const char *, char **, int);
__extension__ unsigned long long strtouq(const char *, char **, int);
void	 unsetenv(const char *);
#endif
__END_DECLS

#endif /* _STDLIB_H_ */
