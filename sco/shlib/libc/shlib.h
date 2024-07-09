/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI shlib.h,v 2.1 1995/02/03 15:17:56 polk Exp
 */

/* from iBCS2 p 6-51 */

struct sco_stat {
	short		sst_dev;
	unsigned short	sst_ino;
	unsigned short	sst_mode;
	short		sst_nlink;
	unsigned short	sst_uid;
	unsigned short	sst_gid;
	short		sst_rdev;
	long		sst_size;
	long		sst_atime;
	long		sst_mtime;
	long		sst_ctime;
};

extern int sco_nfile;

void set_sco_nfile(void);
__dead void panic(const char *s) __attribute__((__volatile));

__dead void sco__exit(int) __attribute__((__volatile));
int sco_fcntl(int, int, ...);
int sco_fstat(int, struct sco_stat *);
int sco_fstat(int, struct sco_stat *);
int sco_ioctl(int, unsigned long, ...);
int sco_open(const char *, int, ...);
char *sco_sbrk(int);
sig_t sco_signal(int, sig_t);
int sco_sigprocmask(int, const sigset_t *, sigset_t *);
int sco_stat(const char *, struct sco_stat *);
ssize_t sco_write(int, const void *, size_t);
