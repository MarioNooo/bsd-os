/*-
 * Copyright (c) 1999 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI randomd.c,v 1.6 2003/08/20 23:42:46 polk Exp
 */

/*
 * Random number daemon
 *
 * Copyright (c) 1995,1996,1999 Paul R. Borman <prb@krystal.com>
 * All rights reserved.
 *
 * Permission to use, copy, and modify this software without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, THE AUTHOR MAKES NO REPRESENTATION OR WARRANTY
 * OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS
 * FOR ANY PARTICULAR PURPOSE.
 *
 * Continually generate random numbers at a priority level of 15.
 * Read the time of day approximately 60 times a second, using
 * the low order 2 * SHIFT bits of the usec time as random data.  These
 * bits are xored into the current random number, which is first
 * shifted up by SHIFT bits.
 * Each time the random number has been shifted up by at least 32 bits
 * the value is stored in a cyclical buffer of NUMBERS entries.
 * Currently it takes about 1/3 of a second to generate 64 random bits.
 *
 * A PF_LOCAL domain socket is created as /var/run/random.
 * Each time a process connects to /var/run/random it may read 64 bits
 * of random data.  To prevent re-use of random numbers in the case that
 * the cyclical buffer is overrun, the read on /dev/random will block until
 * the generator has had a chance to create a new random number.
 *
 * A random key with DES is used to generate new random numbers when the
 * queue is empty.  The DES seed is updated at 6 times per second, alternating
 * on the upper and lower 32 bits.  The DES key is updated once per cycle
 * of the buffer.  Currently that is about every 11.3 minutes.
 *
 * It should never return 0 as a random number
 *
 * The idea of using the clock comes from Matt Blaze's truerand()
 * function, however, any weakness in this generator should be attributed
 * to me and not to Matt.
 *
 * I also borrowed part of Matt's copyright notice, as it was worded so well.
 */

#define	setkey	setkey_c
#define	random	random_c

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/un.h>
#include <err.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#undef	setkey
#undef	random

#include "md5.h"

#define	NUMBERS	4096			/* Must be a power of 2 */
#define	SHIFT	3
#define	PATH_RANDOMCONF	"/etc/random.conf"

char ks[16][8];
unsigned long desnum[2];
unsigned long md5num[4];
int desready = 0;
int md5ready = 0;
unsigned long numbers[NUMBERS];
unsigned int tail = 0;
unsigned int head = 0;
unsigned long r = 0;
fd_set rset;
fd_set wset;
fd_set aset;
fd_set mset;
fd_set dset;

#define	DES	0x01
#define	ASCII	0x02
#define	STREAM	0x04
#define	MD5	0x08

typedef	struct soket_ soket_t;
struct soket_ {
	soket_t *next;
	int mode;
	char *path;
	int fd;
} *soket;

char *defconf[] = {
	"/var/run/random",
	"/var/run/random.ascii ascii",
	"/var/run/random.stream stream",
	"/var/run/random.string stream ascii md5",
	"/var/run/random.des stream des",
	"/var/run/random.md5 stream md5",
	NULL,
};

void ignore() { ; }
void tick();
void getrandom();
void addconf(char *);
void readconf(char *);

#ifdef  KRBDES
#include <openssl/des.h>

void setkey(char ks[16][8], char key[8])
{
        des_key_schedule *k = (des_key_schedule *)ks;
        des_fixup_key_parity((des_cblock *)key);
        des_key_sched((des_cblock *)key, *k);
}
void endes(char ks[16][8], char key[8])
{
        des_cblock cb;
        des_key_schedule *k = (des_key_schedule *)ks;

        des_ecb_encrypt((des_cblock *)key, &cb, *k, DES_ENCRYPT);
        memcpy(key, &cb, 8);
}
#endif

#if	defined(__i386__)
#define cycle_counter(m) asm volatile ("rdtsc; movl %%eax,%0; movl %%edx,4+%0" \
    : "=m" (m) : : "eax", "edx")
#else
#define	cycle_counter(m)	m = 0
#endif

void
nordtsc()
{
	exit (1);
}

int use_cycle_counter = -1;
int verbose = 0;
char *configfile;

main(int ac, char **av)
{
	struct sockaddr_un sun;
	struct sockaddr *sa = (struct sockaddr *)&sun;
	struct sigaction act;
	struct itimerval itv;
	struct stat sb;
	soket_t *s;
	int i;
	u_quad_t sc;
	pid_t pid;
	int status;
	int fdmask = 0;

	while ((i = getopt(ac, av, "cf:tv")) != EOF) {
		switch (i) {
		case 'c':
			use_cycle_counter = 1;
			break;

		case 'f':
			configfile = optarg;
			break;

		case 't':
			use_cycle_counter = 0;
			break;

		case 'v':
			++verbose;
			break;

		default: usage:
			fprintf(stderr, "usage: randomd [-ctv]\n");
			exit(1);
		}
	}

	if (use_cycle_counter == -1) {
		switch ((pid = fork())) {
		case -1:
			use_cycle_counter = 0;
			warn("running without rdtsc");
			break;

		case 0:
			signal(SIGILL, nordtsc);
			cycle_counter(sc);
			if (sc == 0)
				nordtsc();
			exit (0);

		default:
			if (waitpid(pid, &status, 0) == -1) {
				use_cycle_counter = 0;
				warn("running without rdtsc");
			} else if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) == 0)
					use_cycle_counter = 1;
				else
					use_cycle_counter = 0;
			} else {
				use_cycle_counter = 0;
				warnx("running without rdtsc");
			}
			break;
		}
		if (verbose)
			printf("%ssing the system cycle counter\n",
			    use_cycle_counter ? "U" : "Not u");
	}

	/*
	 * if they are using the cycle counter, make sure it works!
	 */
	if (use_cycle_counter) {
		cycle_counter(sc);
		if (sc == 0)
			errx(1, "No system cycle counter available");
	}

	readconf(configfile);
	if (soket == NULL)
		errx(1, "No random sockets defined");

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&aset);
	FD_ZERO(&mset);
	FD_ZERO(&dset);

	if (!verbose)
		daemon(0, 1);

	for (s = soket; s; s = s->next) {
		if ((s->fd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0)
			err(1, "socket");

		fdmask |= (1 << s->fd);
		sun.sun_family = PF_LOCAL;
		strcpy(sun.sun_path, s->path);

		if (!(bind(s->fd, sa, sizeof(sun)) == 0 ||
		    errno == EADDRINUSE &&
		    connect(s->fd, sa, sizeof(sun)) < 0 &&
		    errno == ECONNREFUSED &&
		    unlink(sun.sun_path) == 0 &&
		    bind(s->fd, sa, sizeof(sun)) == 0))
			err(1, "%s", sun.sun_path);
		chmod(s->path, 0644);	/* Only we can write it */
		listen(s->fd, 64);

		if (fcntl(s->fd, F_SETOWN, getpid()))
			err(1, "setown");
		if (fcntl(s->fd, F_SETFL, O_ASYNC))
			err(1, "setfl");
		FD_SET(s->fd, &rset);
	}
	if ((fdmask & (1 << 0)) == 0)
		close(0);
	if (!verbose) {
		if ((fdmask & (1 << 1)) == 0)
			close(1);
		if ((fdmask & (1 << 2)) == 0)
			close(2);
	}

	act.sa_handler = ignore;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGPIPE, &act, 0);

	act.sa_handler = tick;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGALRM);
	sigaddset(&act.sa_mask, SIGIO);
	sigaction(SIGALRM, &act, 0);

	act.sa_handler = getrandom;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGIO);
	sigaction(SIGIO, &act, 0);

	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = use_cycle_counter ? 1000 : 16665;
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = use_cycle_counter ? 1000 : 16665;
	setitimer(0, &itv, 0);

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGALRM);
	sigaddset(&act.sa_mask, SIGIO);
	sigprocmask(SIG_BLOCK, &act.sa_mask, 0);

	setpriority(PRIO_PROCESS, getpid(), 15);

	sigemptyset(&act.sa_mask);
	for (;;)
		sigsuspend(&act.sa_mask);
}

void
tick()
{
	static int i = 0;
	int j;
	int bits;
	struct timeval tv;
	u_quad_t sc;
	
	if (use_cycle_counter) {
		cycle_counter(sc);
		bits = (sc >> 3) & 0xFF;
	} else {
		gettimeofday(&tv, NULL);
		bits = 0;

		for (j = 0; j < 16; j += SHIFT)
			bits ^= (tv.tv_usec >> j) & ((1 << (2 * SHIFT)) - 1);
	}
	r <<= SHIFT;
	r ^= bits;


	if (++i >= 32 / SHIFT) {
		i = 0;
#ifdef	KRBDES
		desnum[head&1] = r;
#endif
		md5num[head&3] = r;
		numbers[head++] = r;
#ifdef	KRBDES
		if (head == 2) {
			setkey(ks, (char *)numbers);
			desready = 1;
		}
#endif
		if (head == 4)
			md5ready = 1;
		r = 0;
		head &= NUMBERS - 1;
		if (head == tail)
			tail = (tail + 1) & (NUMBERS - 1);
	}
}

static unsigned long
random(int mode)
{
	unsigned long n;
	MD5_CTX md5;

	/*
	 * You might think we need to block SIGALRM so that tick() does
	 * not modify something.  We don't need to because:
	 *
    	 * If tick() modifies desnum or md5num then we just work on a
	 * different random number.
	 *
	 * If tail == head then tick() we just wait until it it adds
	 * a new number (if needed).  Otherwise, if tick() does modify
	 * tail we still really don't care, it only moves tail forward
	 * to a previously unused random number.
	 */
	do {
		if (tail == head) {
			if ((mode & MD5) && md5ready) {
				MD5Init(&md5);
				MD5Update(&md5, md5num, sizeof(md5num));
				MD5Final(md5num, &md5);
				n = md5num[0];
			}
#ifdef	KRBDES
			else if ((mode & DES) && desready) {
				endes(ks, (char *)desnum);
				n = desnum[0];
			}
#endif
			else {
				pause();
				n = 0;
			}
		} else {
			n = numbers[tail];
			tail = (tail + 1) & (NUMBERS - 1);
		}
	} while (n == 0);
	return(n);
}

static void
sendrandom(int fd, int mode)
{
	int error;
	unsigned long n0, n1;
	unsigned char buf[18];

	n0 = random(mode);
	n1 = random(mode);

	if (mode & ASCII) {
		sprintf((char *)buf, "%08x%08x\n", n0, n1);
		error = write(fd, buf, 17) <= 0;
	} else {
		buf[0] = (n0      );
		buf[1] = (n0 >>  8);
		buf[2] = (n0 >> 16);
		buf[3] = (n0 >> 24);
		buf[4] = (n1      );
		buf[5] = (n1 >>  8);
		buf[6] = (n1 >> 16);
		buf[7] = (n1 >> 24);
		error = write(fd, buf, 8) <= 0;
	}

	if (error || (mode & STREAM) == 0) {
		FD_CLR(fd, &wset);
		FD_CLR(fd, &aset);
		FD_CLR(fd, &mset);
		FD_CLR(fd, &dset);
		close(fd);
	} else {
		if (mode & ASCII)
			FD_SET(fd, &aset);
		if (mode & DES)
			FD_SET(fd, &dset);
		if (mode & MD5)
			FD_SET(fd, &mset);
		FD_SET(fd, &wset);
	}
}

#define	modes(s) \
	((FD_ISSET(s, &aset) ? ASCII : 0) | \
	 (FD_ISSET(s, &dset) ? DES : 0) | \
	 (FD_ISSET(s, &mset) ? MD5 : 0))

void
getrandom()
{
	int a, i;
	soket_t *s;
	struct timeval tv;
	fd_set fr, fw;

	setpriority(PRIO_PROCESS, getpid(), -4);

	fr = rset;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	while ((fr = rset), (fw = wset),
	    select(FD_SETSIZE, &fr, &fw, 0, &tv) > 0) {
		for (i = 0; i < FD_SETSIZE; ++i) {
			if (!FD_ISSET(i, &fw))
				continue;
			sendrandom(i, STREAM | modes(i));
		}
		for (s = soket; s; s = s->next) {
			if (!FD_ISSET(s->fd, &fr))
				continue;
			if ((a = accept(s->fd, 0, 0)) >= 0)
				sendrandom(a, s->mode);
		}
	}
	setpriority(PRIO_PROCESS, getpid(), 15);
}

void
readconf(char *conf)
{
	FILE *fp;
	int i;
	char *line;
	size_t len;

	if (conf == NULL) {
		if ((fp = fopen(PATH_RANDOMCONF, "r")) == NULL) {
			for (i = 0; defconf[i]; ++i)
				addconf(defconf[i]);
			return;
		}
	} else if ((fp = fopen(conf, "r")) == NULL)
		err(1, "%s", conf);

	while ((line = fgetln(fp, &len)) != NULL) {
		while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
			--len;
		line[len] = '\0';

		while (isspace(*line))
			++line;

		if (line[0] == '\0' || line[0] == '#')
			continue;
		addconf(line);
	}
	fclose(fp);
}

void
addconf(char *line)
{
	char *option;
	char *e;
	soket_t *s;

	s = malloc(sizeof(soket_t));
	if (s == NULL)
		err(1, NULL);
	if ((line = strdup(line)) == NULL)
		err(1, NULL);

	s->next = NULL;
	s->fd = -1;
	s->mode = 0;
	s->path = line;
	while (*line && !isblank(*line))
		++line;
	if (*line) {
		*line++ = '\0';
		while (isblank(*line))
			++line;
	}

	while (*line) {
		option = line;
		while (*line && !isblank(*line))
			++line;
		if (*line) {
			*line++ = '\0';
			while (isblank(*line))
				++line;
		}
		if (strcasecmp(option, "des") == 0)
			s->mode |= DES;
		else if (strcasecmp(option, "md5") == 0)
			s->mode |= MD5;
		else if (strcasecmp(option, "ascii") == 0)
			s->mode |= ASCII;
		else if (strcasecmp(option, "stream") == 0)
			s->mode |= STREAM;
		else
			errx(1, "%s: invalid option", option);
	}
	if (soket == NULL) {
		soket = s;
		return;
	}
	for (s->next = soket; s->next->next; s->next = s->next->next)
		;
	s->next->next = s;
	s->next = NULL;
}
