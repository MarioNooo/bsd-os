/*	BSDI syslog.c,v 2.7 2000/04/08 23:56:04 jch Exp	*/

/*
 * Copyright (c) 1983, 1988, 1993
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
static char sccsid[] = "@(#)syslog.c	8.5 (Berkeley) 4/29/95";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netdb.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vis.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#define	MAXLINE	1024			/* Maximum line before control character expansion */

/* Global context */
static int	LogFile = -1;		/* fd for log */
static int	LogStat = 0;		/* status bits, set by openlog() */
static int	LogFacility = LOG_USER;	/* default facility code */
static int	LogMask = 0xff;		/* mask of priorities to be logged */
static const char *LogTag = NULL;	/* string to tag the entry with */


#define	IOVEC_PRI	0		/* Priority for syslog */
#define	IOVEC_TIME	1		/* Time - start for console */
#define	IOVEC_SPLIT	2		/* Split/continued message indication */
#define	IOVEC_MSG	3		/* Message itself */
#define	IOVEC_MSG2	4		/* continuations if control characters */
#define	IOVEC_TERM	5		/* Message termination */
#define	IOVEC_MAX	6

struct context {
	struct	iovec ctx_iov[IOVEC_MAX];	/* for writing message */
	int	ctx_first;			/* To detect continuation */
};

extern char	*__progname;		/* Program name, from crt0. */

/*
 * syslog, vsyslog --
 *	print message on log file; output is intended for syslogd(8).
 */
void
#if __STDC__
syslog(int pri, const char *fmt, ...)
#else
syslog(pri, fmt, va_alist)
	int pri;
	char *fmt;
	va_dcl
#endif
{
	va_list ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

/*
 * Write routine catches formatted output on FileFp and writes it on
 * the Unix domain socket to the syslog daemon.  If that fails and
 * LOG_CONS is set, write it on the console.  Also write it to stderr
 * if LOG_PERROR is set.  The last two require a scan for control
 * characters to prevent screens from getting screwed up.
 *
 * We rely on stdio to split the message for us (i.e. when the buffer
 * fills up and syslog_write is called).  The second and consecutive
 * pieces of the message are prefixed with [continued message].  The
 * idea here is not to look pretty, but to make an attempt to log all
 * the data in the event that this is a malicious act.
 */
static int
syslog_write(cookie, buf, len)
	void *cookie;
	const char *buf;
	int len;
{
	struct msghdr msghdr;
	struct context *cp;
	int log_cons, log_perror;

	cp = (struct context *)cookie;

	log_perror = (LogStat & LOG_PERROR) != 0;
	log_cons = (LogStat & LOG_CONS) != 0;

	/* Add this data to the iovec */
	cp->ctx_iov[IOVEC_MSG].iov_base = (char *) buf;
	cp->ctx_iov[IOVEC_MSG].iov_len = len;

	/* Add terminating NULL */
	cp->ctx_iov[IOVEC_TERM].iov_base = "\0";
	cp->ctx_iov[IOVEC_TERM].iov_len = 1;

	/* Setup the msghdr */
	memset(&msghdr, 0, sizeof msghdr);
	msghdr.msg_iov = cp->ctx_iov;
	msghdr.msg_iovlen = IOVEC_MAX;

	/*
	 * Attempt to send message, if not open we will re-attempt
	 * open and try again.  Log to the console on failure if
	 * requested.
	 */
	if (sendmsg(LogFile, &msghdr, 0) < 0) {
		closelog();
		openlog(NULL, LogStat | LOG_NDELAY, 0);
		if (sendmsg(LogFile, &msghdr, 0) >= 0)
			log_cons = 0;
	} else
		log_cons = 0;

	/* Log to console and stderr if necessary */
	if (log_cons || log_perror) {
		int c, fd;
		int cc_scan;
		const char *lp;
		const char *p;
		char *q;

		/* Eliminate trailing newline */
		if (len > 1 && buf[len - 1] == '\n') 
			cp->ctx_iov[IOVEC_MSG].iov_len--;
		
		/* Scan for control characters that would need expansion */
		cc_scan = 0;
		for (p = buf, lp = p + cp->ctx_iov[IOVEC_MSG].iov_len; p < lp; p++) {
			c = *p;
			if (!isblank(c) && !isprint(c)) {
				break;
			}
		}

		/* Expand control characters into a malloc'd buffer */
		if (p < lp) {
			cp->ctx_iov[IOVEC_MSG].iov_len = p - buf;

			q = (char *)malloc((lp - p) << 2 + 1);
			if (q == NULL)
				return len;

			cp->ctx_iov[IOVEC_MSG2].iov_base = q;
			cp->ctx_iov[IOVEC_MSG2].iov_len =
			    strvisx(q, p, lp - p, VIS_NL|VIS_CSTYLE|VIS_OCTAL);
		}

		/* Output the message to stderr */
		if (log_perror) {
			cp->ctx_iov[IOVEC_TERM].iov_base = "\n";
			cp->ctx_iov[IOVEC_TERM].iov_len = 1;
			(void)writev(STDERR_FILENO,
				     cp->ctx_iov + IOVEC_SPLIT,
				     IOVEC_MAX - IOVEC_SPLIT);
		}

		/*
		 * Output the message to the console; don't worry about blocking,
		 * if console blocks everything will.  Make sure the error reported
		 * is the one from the syslogd failure.
		 */
		if (log_cons
		    && (fd = open(_PATH_CONSOLE, O_WRONLY, 0)) >= 0) {
			cp->ctx_iov[IOVEC_TERM].iov_base = "\r\n";
			cp->ctx_iov[IOVEC_TERM].iov_len = 2;
			(void)writev(fd, cp->ctx_iov + IOVEC_TIME,
				     IOVEC_MAX - IOVEC_TIME);
			(void)close(fd);
		}

		/* Free the control character expansion buffer */
		if (cp->ctx_iov[IOVEC_MSG2].iov_base) {
			free(cp->ctx_iov[IOVEC_MSG2].iov_base);
			cp->ctx_iov[IOVEC_MSG2].iov_base = NULL;
			cp->ctx_iov[IOVEC_MSG2].iov_len = 0;
		}
        }

	if (cp->ctx_first) {
		/* Continuation of previous message */

		cp->ctx_first = 0;
		cp->ctx_iov[IOVEC_SPLIT].iov_base = "[continued message] ";
		cp->ctx_iov[IOVEC_SPLIT].iov_len = strlen(cp->ctx_iov[IOVEC_SPLIT].iov_base);
	}

	return len;
}


void
vsyslog(pri, fmt, ap)
	int pri;
	register const char *fmt;
	va_list ap;
{
	struct context ctx;
	time_t now;
	FILE *fp;
	char ch, *t, *lp;
	char *saved_error;
	char fmt_cpy[1024];	/* To store editted format string */
	char pri_buf[16];	/* To store priority as text */
	char time_buf[32];	/* To store time string */

	saved_error = strerror(errno);

#define	INTERNALLOG	LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
	/* Check for invalid bits. */
	if (pri & ~(LOG_PRIMASK|LOG_FACMASK)) {
		syslog(INTERNALLOG,
		    "syslog: unknown facility/priority: %x", pri);
		pri &= LOG_PRIMASK|LOG_FACMASK;
	}

	/* Check priority against setlogmask values. */
	if (!(LOG_MASK(LOG_PRI(pri)) & LogMask))
		return;

	/* Set default facility if none specified. */
	if ((pri & LOG_FACMASK) == 0)
		pri |= LogFacility;

	/* Substitute error message for %m. */
	lp = &fmt_cpy[sizeof fmt_cpy - 1];
	for (t = fmt_cpy; t < lp && (ch = *fmt); ++fmt) {
		if (ch == '%') {
			/* must handle %% specially to avoid "1%%m", e.g. */
			if (fmt[1] == '%') {
				++fmt;
				if (t < lp - 1) {
					*t++ = '%';
					*t++ = '%';
				}
				continue;
			}
			if (fmt[1] == 'm') {
				int l, r;

				++fmt;
				r = snprintf(t, (l = lp - t), "%s",
					     saved_error);
				t += r < l ? r : l - 1;
				continue;
			}
			/* other % escapes copied through below */
		}
		*t++ = ch;
	}
	*t = '\0';

	/* Insure that we are connected */
	if (LogFile == -1)
		openlog(LogTag ? LogTag : __progname,
			LogStat | LOG_NDELAY,
			0);

	/* For messages that are too long */
	ctx.ctx_first = 1;

	/* Initialize iovec */
	memset(&ctx.ctx_iov, 0, sizeof ctx.ctx_iov);

	/* Format the priority */
	ctx.ctx_iov[IOVEC_PRI].iov_base = pri_buf;
	ctx.ctx_iov[IOVEC_PRI].iov_len =
	    snprintf(pri_buf, sizeof pri_buf, "<%d>", pri);

	/* Formate the date and time */
	(void)time(&now);
	ctx.ctx_iov[IOVEC_TIME].iov_base = time_buf;
	ctx.ctx_iov[IOVEC_TIME].iov_len =
	    strftime(time_buf, sizeof time_buf, "%h %e %T ", localtime(&now));

	/* Open our internal stream */
	fp = fwopen(&ctx, syslog_write);
	if (fp == NULL)
		return;

	(void)setvbuf(fp, NULL, _IOFBF, MAXLINE);

	/* Add the Tag and PID */
	if (LogStat & LOG_PID)
		(void)fprintf(fp, "%s[%ld]: ",
			      LogTag ? LogTag : "",
			      getpid());
	else
		(void)fprintf(fp, "%s: ",
			      LogTag ? LogTag : "");

	/* Format the message */
	(void)vfprintf(fp, fmt_cpy, ap);

	/* And close the stream */
	(void)fclose(fp);
}

void
openlog(ident, logstat, logfac)
	const char *ident;
	int logstat, logfac;
{
	int fd, newfd, saverr;
	struct sockaddr_un addr;
#define	MIN_FILENO (STDERR_FILENO + 1)

	if (ident != NULL)
		LogTag = ident;
	LogStat = logstat;
	if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0)
		LogFacility = logfac;

	if (LogFile == -1 && LogStat & LOG_NDELAY) {
		/* Open log now */

		/*
		 * This is a configuration error and should never occur,
		 * but we will test for it and fail rather than overwriting
		 * the stack in the strcpy() below.
		 */
		if (sizeof _PATH_LOG > sizeof addr.sun_path) {
			errno = ENAMETOOLONG;	/* seems closest */
			return;
		}

		if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
			return;

		if (fd < MIN_FILENO) {
			newfd = fcntl(fd, F_DUPFD, MIN_FILENO);
			saverr = errno;
			(void)close(fd);
			errno = saverr;
			if (newfd < 0)
				return;
			fd = newfd;
		}

		/* Set close-on-exec */
		(void)fcntl(fd, F_SETFD, FD_CLOEXEC);

		(void)strcpy(addr.sun_path, _PATH_LOG);
		addr.sun_family = AF_LOCAL;
		if (connect(fd, (struct sockaddr *)&addr, SUN_LEN(&addr))) {
			saverr = errno;
			(void)close(fd);
			errno = saverr;
			return;
		}
		LogFile = fd;
	}
}

void
closelog()
{

	if (LogFile != -1) {
		(void)close(LogFile);
		LogFile = -1;
	}
}

/* setlogmask -- set the log mask level */
int
setlogmask(pmask)
	int pmask;
{
	int omask;

	omask = LogMask;
	if (pmask != 0)
		LogMask = pmask;
	return (omask);
}
