/*
 * Copyright (c) 1992, 1993, 1996
 *	Berkeley Software Design, Inc.  All rights reserved.
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
 *	This product includes software developed by Berkeley Software
 *	Design, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY Berkeley Software Design, Inc. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Berkeley Software Design, Inc. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI AsyncIO.c,v 2.3 1998/01/29 16:51:43 donn Exp
 */
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include "doscmd.h"

#define FD_ISZERO(p)	((p)->fds_bits[0] == 0)

static	sigset_t	_sigio;
static	int		_sigio_set = 0;
#define	ENABLE_IO	{ async_io = 1; sigprocmask(SIG_UNBLOCK, &_sigio, 0); }
#define	DISABLE_IO	{ sigprocmask(SIG_BLOCK, &_sigio, 0); async_io = 0; }

/*
 * Set or Clear the Async nature of an FD
 */

#define	SETASYNC(fd)	fcntl(fd, F_SETFL, handlers[fd].flag | FASYNC)
#define	CLRASYNC(fd)	fcntl(fd, F_SETFL, handlers[fd].flag & ~FASYNC)

/*
 * Request that ``func'' be called everytime data is available on ``fd''
 */

static	fd_set	fdset;			/* File Descriptors to select on */

typedef	struct {
	void	(*func)();		/* Function to call when data arrives */
	void	(*failure)();		/* Function to call on failure */
	void	*arg;			/* Argument to above functions */
	int	lockcnt;		/* Nested level of lock */
	fd_set	members;		/* Set of FD's to disable on SIGIO */
	int	flag;			/* The flag from F_GETFL (we own it) */
} Async;

static	Async	handlers[OPEN_MAX];

static	void	HandleIO (struct sigframe *sf, struct trapframe *tf);

static 	int	in_handler = 0;
static	int	async_io = 0;

void
_RegisterIO(fd, func, arg, failure)
int fd;
void (*func)();
void *arg;
void (*failure)();
{
	static int firsttime = 1;
	Async *as;

	if (fd < 0 || fd > OPEN_MAX) {
printf("%d: Invalid FD\n", fd);
		return;
	}

	as = &handlers[fd];

        if ((as->flag = fcntl(fd, F_GETFL, 0)) == -1) {
		if (func) {
/*@*/			perror("get fcntl");
/*@*/			abort();
			return;
		}
        }

	if (!_sigio_set) {
		sigemptyset(&_sigio);
		sigaddset(&_sigio, SIGIO);
		_sigio_set = 1;
	}
	if (firsttime) {
#if 0
		struct sigaction sa;
#endif

		firsttime = 0;
		async_io = 1;
    	    	setsignal(SIGIO, HandleIO);
#if 0
		sa.sa_handler = HandleIO;
		sa.sa_mask = sigmask(SIGIO) | sigmask(SIGALRM);
		sa.sa_flags = SA_ONSTACK;
		sigaction(SIGIO, &sa, NULL);
#endif
	}

	if (handlers[fd].func = func) {
		as->lockcnt = 0;
		as->arg = arg;
		as->failure = failure;

		FD_SET(fd, &fdset);
		FD_ZERO(&handlers[fd].members);
		FD_SET(fd, &handlers[fd].members);
		if (fcntl(fd, F_SETOWN, getpid()) < 0) {
/*@*/			perror("SETOWN");
		}
		SETASYNC(fd);
	} else {
		as->arg = 0;
		as->failure = 0;
		as->lockcnt = 0;

		CLRASYNC(fd);
		FD_CLR(fd, &fdset);
	}

	/*
	 * Now depending on the state of fdset, we either turn SIGIO
	 * on or off.
	 */
	if (FD_ISZERO(&fdset)) {
		DISABLE_IO
	} else {
		ENABLE_IO
	}
}

static void
CleanIO()
{
	int x;
	static struct timeval tv = { 0 };

	/*
	 * For every file des in fd_set, we check to see if it
	 * causes a fault on select().  If so, we unregister it
	 * for the user.
	 */
	for (x = 0; x < OPEN_MAX; ++x) {
		fd_set set;

		if (!FD_ISSET(x, &fdset))
			continue;

		FD_ZERO(&set);
		FD_SET(x, &set);
		errno = 0;
		if (select(FD_SETSIZE, &set, 0, 0, &tv) < 0 &&
		    errno == EBADF) {
			void (*f)();
			void *a;
printf("Closed file descriptor %d\n", x);

			f = handlers[x].failure;
			a = handlers[x].arg;
			handlers[x].failure = 0;
			handlers[x].func = 0;
			handlers[x].arg = 0;
			handlers[x].lockcnt = 0;
			FD_CLR(x, &fdset);
			if (f)
				(*f)(a);
		}
	}
}

static void
HandleIO(struct sigframe *sf, struct trapframe *tf)
{
	async_io = 0;
	++in_handler;

	for (;;) {
		static struct timeval tv = { 0 };
		fd_set readset;
		int x;
		int fd;

		readset = fdset;
		if ((x = select(FD_SETSIZE, &readset, 0, 0, &tv)) < 0) {
			/*
			 * If we failed becuase of a BADFiledes, go find
			 * which one(s), fail them out and then try a
			 * new select to see if any of the good ones are
			 * okay.
			 */
			if (errno == EBADF) {
				CleanIO();
				if (FD_ISZERO(&fdset))
					break;
				continue;
			}
			perror("select");
			break;
		}

		/*
		 * If we run out of fds to look at, break out of the loop
		 * and exit the handler.
		 */
		if (!x)
			break;

		/*
		 * If there is at least 1 fd saying it has something for
		 * us, then loop through the sets looking for those
		 * bits, stopping when we have handleed the number it has
		 * asked for.
		 */
		for (fd = 0; x && fd < OPEN_MAX; ++fd) {
			Async *as;

			if (!FD_ISSET(fd, &readset)) {
				continue;
			}
			--x;

			/*
			 * Is suppose it is possible that one of the previous
			 * io requests changed the fdset.
			 * We do know that SIGIO is turned off right now,
			 * so it is safe to checkit.
			 */
			if (!FD_ISSET(fd, &fdset)) {
				continue;
			}
			as = &handlers[fd];

			/*
			 * as in above, maybe someone locked us...
			 * we are in dangerous water now if we are
			 * multi-tasked
			 */
			if (as->lockcnt) {
/*@*/			fprintf(stderr, "Selected IO on locked %d\n",fd);
				continue;
			}
			/*
			 * Okay, now if there exists a handler, we should
			 * call it.  We must turn back on SIGIO if there
			 * are possibly other people waiting for it.
			 */
			if (as->func) {
				int afd;
				Async *aas;

				/*
				 * STEP 1: Lock out all "members"
				 */
				aas = handlers;
if (0)
				for (afd = 0; afd < OPEN_MAX; ++afd, ++aas) {
				    if (FD_ISSET(afd, &as->members)) {
					if (aas->func) {
					    if (as->lockcnt++ == 0) {
						FD_CLR(afd, &fdset);
						CLRASYNC(afd);
					    }
					
					}
				    }
				}

				/*
				 * STEP 2: Renable SIGIO so other FDs can
				 *         use a hit.
				ENABLE_IO
				 */

				/*
				 * STEP 3: Call the handler
				 */
				(*handlers[fd].func)(handlers[fd].arg, sf, tf);

				/*
				 * STEP 4: Just turn SIGIO off.  No check.
				DISABLE_IO
				 */

				/*
				 * STEP 5: Unlock all "members"
				 */
				aas = handlers;
if (0)
				for (afd = 0; afd < OPEN_MAX; ++afd, ++aas) {
				    if (FD_ISSET(afd, &as->members)) {
					if (aas->func) {
					    if (--as->lockcnt == 0) {
						FD_SET(afd, &fdset);
						SETASYNC(afd);
					    }
					}
				    }
				}
			} else {
				/*
				 * Otherwise deregister this guy.
				 */
				_RegisterIO(fd, 0, 0, 0);
			}
		}
		/*
		 * If we did not process all the fd's, then we should
		 * break out of the probable infinite loop.
		 */
		if (x) {
			break;
		}
	}
	--in_handler;
	async_io = 1;
}

static sigset_t stack[64];
static stackp = 0;

void
_BlockIO()
{
	if (stackp >= 64) {
		fprintf(stderr, "Signal stack count to deep\n");
		abort();
	}
	if (!_sigio_set) {
		sigemptyset(&_sigio);
		sigaddset(&_sigio, SIGIO);
		sigaddset(&_sigio, SIGALRM);
		_sigio_set = 1;
	}
	sigprocmask(SIG_BLOCK, &_sigio, &stack[stackp++]);
}

void
_UnblockIO()
{
	if (stackp <= 0) {
		return;
		fprintf(stderr, "Negative signal stack count\n");
		abort();
	}
	sigprocmask(SIG_SETMASK, &stack[--stackp], 0);
}