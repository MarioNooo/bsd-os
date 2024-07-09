#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/ftp.h>
#include "ftp_var.h"

#ifndef	SECSPERHOUR
#define	SECSPERHOUR	(60*60)
#endif

/*
 * determine size of remote file
 */
off_t
remotesize(file, noisy)
	const char *file;
	int noisy;
{
	int overbose;
	off_t size;

	overbose = verbose;
	size = -1;
	if (debug == 0)
		verbose = -1;
	if (command("SIZE %s", file) == COMPLETE) {
		char *cp, *ep;

		cp = strchr(reply_string, ' ');
		if (cp != NULL) {
			cp++;
			size = strtoq(cp, &ep, 10);
			if (*ep != '\0' && !isspace((unsigned char)*ep))
				size = -1;
		}
	} else if (noisy && debug == 0)
		puts(reply_string);
	verbose = overbose;
	return (size);
}

/*
 * determine last modification time (in GMT) of remote file
 */
time_t
remotemodtime(file, noisy)
        const char *file;
        int noisy;
{
	struct tm timebuf;
	time_t rtime;
	int n, len, month, ocode, overbose, y2kbug, year;
	char *fmt;
	char mtbuf[17], tbuf[16];
		
	overbose = verbose;
	ocode = code;
	rtime = -1;
	if (debug == 0)
		verbose = -1;
	if (command("MDTM %s", file) == COMPLETE) {
		memset(&timebuf, 0, sizeof(timebuf));
		/*
		 * Parse the time string, which is expected to be 14
		 * characters long plus an optional '.' and trailing
		 * fractional second.  Some broken servers send tm_year
		 * formatted with "19%02d", which produces an incorrect
		 * (but parsable) 15 characters for years >= 2000.
		 */
		n = sscanf(reply_string, "%*s %16[0-9]%15s", mtbuf, tbuf);
		if (n == 1 || (n == 2 && tbuf[0] == '.')) {
			fmt = NULL;
			len = strlen(mtbuf);
			y2kbug = 0;
			if (len == 15 && strncmp(mtbuf, "19", 2) == 0) {
				fmt = "19%03d%02d%02d%02d%02d%02d";
				y2kbug = 1;
			} else if (len == 14)
				fmt = "%04d%02d%02d%02d%02d%02d";
			if (fmt != NULL) {
				if (sscanf(mtbuf, fmt, &year, &month,
				    &timebuf.tm_mday, &timebuf.tm_hour,
				    &timebuf.tm_min, &timebuf.tm_sec) == 6) {
					timebuf.tm_isdst = -1;
					timebuf.tm_mon = month - 1;
					if (y2kbug)
						timebuf.tm_year = year;
					else
						timebuf.tm_year = year - 1900;
					rtime = mktime(&timebuf);
				}
			}
		}
		if (rtime == -1) {
			if (noisy || debug != 0)
				printf("Can't convert %s to a time.\n", mtbuf);
		} else
			rtime += timebuf.tm_gmtoff;     /* conv. local -> GMT */
	} else if (noisy && debug == 0)
		puts(reply_string);
	verbose = overbose;
	if (rtime == -1)
		code = ocode;
	return (rtime);
}

void
timersub(t1, t0, tdiff)
	struct timeval *t1, *t0, *tdiff;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

void updateprogressmeter __P((int));

void
updateprogressmeter(dummy)
        int dummy;
{
        static pid_t pgrp = -1;
        int ctty_pgrp;

        if (pgrp == -1)
                pgrp = getpgrp();

        /*
         * print progress bar only if we are foreground process.
         */
        if (ioctl(STDOUT_FILENO, TIOCGPGRP, &ctty_pgrp) != -1 &&
            ctty_pgrp == (int)pgrp)
                progressmeter(0);
}

/*
 * Display a transfer progress bar if progress is non-zero.
 * SIGALRM is hijacked for use by this function.
 * - Before the transfer, set filesize to size of file (or -1 if unknown),
 *   and call with flag = -1. This starts the once per second timer,
 *   and a call to updateprogressmeter() upon SIGALRM.
 * - During the transfer, updateprogressmeter will call progressmeter
 *   with flag = 0
 * - After the transfer, call with flag = 1
 */
static struct timeval start;

void
progressmeter(flag)
        int flag;
{
	/*
	 * List of order of magnitude prefixes.
	 * The last is `P', as 2^64 = 16384 Petabytes
	 */
	static const char prefixes[] = " KMGTP";

	static struct timeval lastupdate;
	static off_t lastsize;
	struct timeval now, td, wait;
	off_t cursize, abbrevsize;
	double elapsed;
	int ratio, barlength, i, len, n;
	off_t remaining;
	char buf[256];

	len = 0;

	if (flag == -1) {
		(void)gettimeofday(&start, (struct timezone *)0);
		lastupdate = start;
		lastsize = restart_point;
	}
	(void)gettimeofday(&now, (struct timezone *)0);
	if (!progress || filesize <= 0)
		return;
	cursize = bytes + restart_point;

	ratio = cursize * 100 / filesize;
	ratio = MAX(ratio, 0);
	ratio = MIN(ratio, 100);
	n = snprintf(buf + len, sizeof(buf) - len, "\r%3d%% ", ratio);
	if (n > 0 && n < sizeof(buf) - len)
		len += n;

	barlength = ttywidth - 30;
	if (barlength > 0) {
		if (barlength > 154)
			barlength = 154;	/* Number of '*'s below */
		i = barlength * ratio / 100;
		n = snprintf(buf + len, sizeof(buf) - len,
		    "|%.*s%*s|", i,
"*****************************************************************************"
"*****************************************************************************",
		    barlength - i, "");
		if (n > 0 && n < sizeof(buf) - len)
			len += n;
	}

	i = 0;
	abbrevsize = cursize;
	while (abbrevsize >= 100000 && i < sizeof(prefixes)) {
		i++;
		abbrevsize >>= 10;
	}
	n = snprintf(buf + len, sizeof(buf) - len,
	    " %5qd %c%c ", (long long)abbrevsize, prefixes[i],
	    prefixes[i] == ' ' ? ' ' : 'B');
	if (n > 0 && n < sizeof(buf) - len)
		len += n;

	timersub(&now, &lastupdate, &wait);
	if (cursize > lastsize) {
		lastupdate = now;
		lastsize = cursize;
		if (wait.tv_sec >= STALLTIME) { /* fudge out stalled time */
			start.tv_sec += wait.tv_sec;
			start.tv_usec += wait.tv_usec;
		}
		wait.tv_sec = 0;
	}

	timersub(&now, &start, &td);
	elapsed = td.tv_sec + (td.tv_usec / 1000000.0);

	if (bytes <= 0 || elapsed <= 0.0 || cursize > filesize) {
		n = snprintf(buf + len, sizeof(buf) - len,
		    "   --:-- ETA");
	} else if (wait.tv_sec >= STALLTIME) {
		n = snprintf(buf + len, sizeof(buf) - len,
		    " - stalled -");
	} else {
		remaining =
		    ((filesize - restart_point) / (bytes / elapsed) - elapsed);
		if (remaining >= 100 * SECSPERHOUR)
			n = snprintf(buf + len, sizeof(buf) - len,
			    "   --:-- ETA");
		else {
			i = remaining / SECSPERHOUR;
			if (i)
				n = snprintf(buf + len, sizeof(buf) - len,
				    "%2d:", i);
			else
				n = snprintf(buf + len, sizeof(buf) - len,
				    "   ");
			if (n > 0 && n < sizeof(buf) - len)
				len += n;
			i = remaining % SECSPERHOUR;
			n = snprintf(buf + len, sizeof(buf) - len,
			    "%02d:%02d ETA", i / 60, i % 60);
		}
	}
	if (n > 0 && n < sizeof(buf) - len)
		len += n;
	(void)write(STDOUT_FILENO, buf, len);

	if (flag == -1) {
		(void)signal(SIGALRM, updateprogressmeter);
		alarmtimer(1);		/* set alarm timer for 1 Hz */
	} else if (flag == 1) {
		alarmtimer(0);
		(void)putchar('\n');
	}
	fflush(stdout);
}

/*
 * Display transfer statistics.
 * Requires start to be initialised by progressmeter(-1),
 * direction to be defined by xfer routines, and filesize and bytes
 * to be updated by xfer routines
 * If siginfo is nonzero, an ETA is displayed, and the output goes to STDERR
 * instead of STDOUT.
 */
void
ptransfer(siginfo)
        int siginfo;
{
	struct timeval now, td;
	double elapsed;
	off_t bs;
	int meg, n, remaining, hh, len;
	char buf[100];

	if (!verbose && !siginfo)
		return;
	
	(void)gettimeofday(&now, (struct timezone *)0);
	timersub(&now, &start, &td);
	elapsed = td.tv_sec + (td.tv_usec / 1000000.0);
	bs = bytes / (elapsed == 0.0 ? 1 : elapsed);
	meg = 0;
	if (bs > (1024 * 1024))
		meg = 1;
	len = 0;
	n = snprintf(buf + len, sizeof(buf) - len,
	    "%qd byte%s %s in %.2f seconds (%.2f %sB/s)\n",
	    (long long)bytes, bytes == 1 ? "" : "s", direction, elapsed,
	    bs / (1024.0 * (meg ? 1024.0 : 1.0)), meg ? "M" : "K");
	if (n > 0 && n < sizeof(buf) - len)
		len += n;
	if (siginfo && bytes > 0 && elapsed > 0.0 && filesize >= 0
	    && bytes + restart_point <= filesize) {
		remaining = (int)((filesize - restart_point) /
				  (bytes / elapsed) - elapsed);
		hh = remaining / SECSPERHOUR;
		remaining %= SECSPERHOUR;
		len--;                  /* decrement len to overwrite \n */
		n = snprintf(buf + len, sizeof(buf) - len,
		    "  ETA: %02d:%02d:%02d\n", hh, remaining / 60,
		    remaining % 60);
		if (n > 0 && n < sizeof(buf) - len)
			len += n;
	}
	(void)write(siginfo ? STDERR_FILENO : STDOUT_FILENO, buf, len);
}

/*
 * Update the global ttywidth value, using TIOCGWINSZ.
 */
void
setttywidth(a)
	int a;
{
	struct winsize winsize;

	if (ioctl(fileno(stdout), TIOCGWINSZ, &winsize) != -1)
		ttywidth = winsize.ws_col;
	else
		ttywidth = 80;
}

/*
 * Set the SIGALRM interval timer for wait seconds, 0 to disable.
 */
void
alarmtimer(wait)
	int wait;
{
	struct itimerval itv;

	itv.it_value.tv_sec = wait;
	itv.it_value.tv_usec = 0;
	itv.it_interval = itv.it_value;
	setitimer(ITIMER_REAL, &itv, NULL);
}
