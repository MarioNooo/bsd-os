/*-
 * Copyright (c) 1991, 1993
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)exec.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <paths.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

extern char **environ;

/*
 * Typically, if an exec fails, the caller will either try another
 * exec or exit, so it may seem odd to keep track of allocated memory
 * here.  Do not take it out lightly, though: if our caller has done
 * a vfork that shares memory modifications, space we allocate in
 * the child will also get allocated in the parent.  Thus, margv
 * below could be non-NULL on a later (second fork/vfork) call, after
 * a child's exec succeeds.
 */
#define	BASE		100		/* Initial allocation. */
#define	FREELIMIT	400		/* Max allocation without free. */

static char **margv;			/* Allocated argv memory. */
static size_t memsize;			/* Size of margv. */

/*
 * Build an argv list (and possibly an envp) from a va_list.  Returns NULL
 * on failure, memory to be free'd otherwise.  (Note, this code depends on
 * free(NULL) being a no-op.)
 */
static int
buildargv(ap, arg, envpp)
	va_list ap;
	const char *arg;
	char ***envpp;
{
	int off;
	char **newp;

	for (off = 0;; ++off) {
		if (off >= memsize) {
			if (memsize == 0)
				memsize = BASE;
			else
				memsize *= 2;		/* Ramp up fast. */
			if ((newp =
			    realloc(margv, memsize * sizeof(char *))) == NULL) {
				memsize = 0;
				free(margv);
				return (1);
			}
			margv = newp;
		}
		if (off == 0)
			margv[0] = (char *)arg;
		else
			if ((margv[off] = va_arg(ap, char *)) == NULL)
				break;
	}
	/* Get environment pointer if user supposed to provide one. */
	if (envpp)
		*envpp = va_arg(ap, char **);
	return (0);
}

int
#if __STDC__
execl(const char *name, const char *arg, ...)
#else
execl(name, arg, va_alist)
	const char *name;
	const char *arg;
	va_dcl
#endif
{
	va_list ap;
	int sverrno;

	if (memsize > FREELIMIT) {
		free(margv);
		memsize = 0;
	}
#if __STDC__
	va_start(ap, arg);
#else
	va_start(ap);
#endif
	if (!buildargv(ap, arg, NULL))
		(void)execve(name, margv, environ);
	va_end(ap);

	sverrno = errno;
	free(margv);
	memsize = 0;
	errno = sverrno;

	return (-1);
}

int
#if __STDC__
execle(const char *name, const char *arg, ...)
#else
execle(name, arg, va_alist)
	const char *name;
	const char *arg;
	va_dcl
#endif
{
	va_list ap;
	int sverrno;
	char **envp;

	if (memsize > FREELIMIT) {
		free(margv);
		memsize = 0;
	}
#if __STDC__
	va_start(ap, arg);
#else
	va_start(ap);
#endif
	if (!buildargv(ap, arg, &envp))
		(void)execve(name, margv, envp);
	va_end(ap);

	sverrno = errno;
	free(margv);
	memsize = 0;
	errno = sverrno;

	return (-1);
}

int
#if __STDC__
execlp(const char *name, const char *arg, ...)
#else
execlp(name, arg, va_alist)
	const char *name;
	const char *arg;
	va_dcl
#endif
{
	va_list ap;
	int sverrno;

	if (memsize > FREELIMIT) {
		free(margv);
		memsize = 0;
	}
#if __STDC__
	va_start(ap, arg);
#else
	va_start(ap);
#endif
	if (!buildargv(ap, arg, NULL))
		(void)execvp(name, margv);
	va_end(ap);

	sverrno = errno;
	free(margv);
	memsize = 0;
	errno = sverrno;

	return (-1);
}

int
execv(name, argv)
	const char *name;
	char * const *argv;
{
	(void)execve(name, argv, environ);
	return (-1);
}

int
execvp(name, argv)
	const char *name;
	char * const *argv;
{
	static int memsize;
	static char **memp;
	register int cnt, lp, ln;
	register char *p;
	int eacces, etxtbsy;
	char *bp, *cur, *path, buf[MAXPATHLEN];

	eacces = etxtbsy = 0;

	/* If it's an absolute or relative path name, it's easy. */
	if (index(name, '/')) {
		bp = (char *)name;
		cur = path = NULL;
		goto retry;
	}
	bp = buf;

	/* Get the path we're searching. */
	if (!(path = getenv("PATH")))
		path = _PATH_DEFPATH;
	cur = alloca(strlen(path) + 1);
	if (cur == NULL) {
		errno = ENOMEM;
		return (-1);
	}
	strcpy(cur, path);

	while ((p = strsep(&cur, ":")) != NULL) {
		/*
		 * It's a SHELL path -- double, leading and trailing colons
		 * mean the current directory.
		 */
		if (!*p) {
			p = ".";
			lp = 1;
		} else
			lp = strlen(p);
		ln = strlen(name);

		/*
		 * If the path is too long complain.  This is a possible
		 * security issue; given a way to make the path too long
		 * the user may execute the wrong program.
		 */
		if (lp + ln + 2 > sizeof(buf)) {
			(void)write(STDERR_FILENO, "execvp: ", 8);
			(void)write(STDERR_FILENO, p, lp);
			(void)write(STDERR_FILENO, ": path too long\n", 16);
			continue;
		}
		bcopy(p, buf, lp);
		buf[lp] = '/';
		bcopy(name, buf + lp + 1, ln);
		buf[lp + ln + 1] = '\0';

retry:		(void)execve(bp, argv, environ);
		switch(errno) {
		case EACCES:
			eacces = 1;
			break;
		case ENOENT:
			break;
		case ENOEXEC:
			for (cnt = 0; argv[cnt]; ++cnt);
			if ((cnt + 2) * sizeof(char *) > memsize) {
				memsize = (cnt + 2) * sizeof(char *);
				if ((memp = realloc(memp, memsize)) == NULL) {
					memsize = 0;
					goto done;
				}
			}
			memp[0] = "sh";
			memp[1] = bp;
			bcopy(argv + 1, memp + 2, cnt * sizeof(char *));
			(void)execve(_PATH_BSHELL, memp, environ);
			goto done;
		case ETXTBSY:
			if (etxtbsy < 3)
				(void)sleep(++etxtbsy);
			goto retry;
		default:
			goto done;
		}
	}
	if (eacces)
		errno = EACCES;
	else if (!errno)
		errno = ENOENT;
done:
	return (-1);
}
