/*	BSDI newfs.c,v 2.12 2002/02/05 15:37:19 giff Exp	*/

/*
 * Copyright (c) 1983, 1989, 1993, 1994
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

#ifndef lint
static char sccsid[] = "@(#)newfs.c	8.13 (Berkeley) 5/1/95";
#endif /* not lint */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

/*
 * newfs: friendly front end to mkfs
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/file.h>
#include <sys/mount.h>

#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ufs/ufsmount.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "mntopts.h"

struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	{ NULL },
};

#if __STDC__
void	fatal(const char *fmt, ...);
#else
void	fatal();
#endif

#define	COMPAT			/* allow non-labeled disks */

/*
 * The following two constants set the default block and fragment sizes.
 * Both constants must be a power of 2 and meet the following constraints:
 *	MINBSIZE <= DESBLKSIZE <= MAXBSIZE
 *	sectorsize <= DESFRAGSIZE <= DESBLKSIZE
 *	DESBLKSIZE / DESFRAGSIZE <= 8
 */
#define	DFL_FRAGSIZE	1024
#define	DFL_BLKSIZE	8192

/*
 * Cylinder groups may have up to many cylinders. The actual
 * number used depends upon how much information can be stored
 * on a single cylinder. The default is to use 16 cylinders
 * per group.
 */
#define	DESCPG		16	/* desired fs_cpg */

/*
 * ROTDELAY gives the minimum number of milliseconds to initiate
 * another disk transfer on the same cylinder. It is used in
 * determining the rotationally optimal layout for disk blocks
 * within a file; the default of fs_rotdelay is 0ms.
 */
#define ROTDELAY	0

/*
 * MAXBLKPG determines the maximum number of data blocks which are
 * placed in a single cylinder group. The default is one indirect
 * block worth of data blocks.
 */
#define MAXBLKPG(bsize)	((bsize) / sizeof(daddr_t))

/*
 * Each file system has a number of inodes statically allocated.
 * We allocate one inode slot per NFPI fragments, expecting this
 * to be far more than we will ever need.
 */
#define	NFPI		4

/*
 * For each cylinder we keep track of the availability of blocks at different
 * rotational positions, so that we can lay out the data to be picked
 * up with minimum rotational latency.  NRPOS is the default number of
 * rotational positions that we distinguish.  With NRPOS of 8 the resolution
 * of our summary information is 2ms for a typical 3600 rpm drive.
 */
#define	NRPOS		8	/* number distinct rotational positions */


int	mfs;			/* run as the memory based filesystem */
int	Nflag;			/* run without writing file system */
int	Oflag;			/* format as an 4.3BSD file system */
int	fssize;			/* file system size */
int	ntracks;		/* # tracks/cylinder */
int	nsectors;		/* # sectors/track */
int	nphyssectors;		/* # sectors/track including spares */
int	secpercyl;		/* sectors per cylinder */
int	trackspares = -1;	/* spare sectors per track */
int	cylspares = -1;		/* spare sectors per cylinder */
int	sectorsize;		/* bytes/sector */
#ifdef tahoe
int	realsectorsize;		/* bytes/sector in hardware */
#endif
int	rpm;			/* revolutions/minute of drive */
int	interleave;		/* hardware sector interleave */
int	trackskew = -1;		/* sector 0 skew, per track */
int	headswitch;		/* head switch time, usec */
int	trackseek;		/* track-to-track seek, usec */
int	fsize = 0;		/* fragment size */
int	bsize = 0;		/* block size */
int	cpg = DESCPG;		/* cylinders/cylinder group */
int	cpgflg;			/* cylinders/cylinder group flag was given */
int	minfree = MINFREE;	/* free space threshold */
int	opt = DEFAULTOPT;	/* optimization preference (space or time) */
int	density;		/* number of bytes per inode */
int	maxcontig = 0;		/* max contiguous blocks to allocate */
int	rotdelay = ROTDELAY;	/* rotational delay between blocks */
int	maxbpg;			/* maximum blocks per file in a cyl group */
int	nrpos = NRPOS;		/* # of distinguished rotational positions */
int	avgfilesize = AVFILESIZ;/* expected average file size */
int	avgfilesperdir = AFPDIR;/* expected number of files per directory */
int	bbsize = BBSIZE;	/* boot block size */
int	sbsize = SBSIZE;	/* superblock size */
int	mntflags = MNT_ASYNC;	/* flags to be passed to mount */
int	dosoftupdates = 1;	/* -1 -> don't, 1 -> do */
caddr_t	membase;		/* start address of memory based filesystem */
#ifdef COMPAT
char	*disktype;
int	unlabeled;
#endif

char	device[MAXPATHLEN];
char	*progname;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	register int ch;
	register struct partition *pp;
	register struct disklabel *lp;
	struct disklabel *getdisklabel();
	struct partition oldpartition;
	struct stat st;
	struct statfs *mp;
	int dumpit, initit, fsi, fso, len, n;
	char *dumpf, *initf;
	char *cp, *s1, *s2, *special, *opstring, buf[BUFSIZ];
	char *specname, *mntname;

	dumpit = -1;
	initit = -1;

	if (progname = strrchr(*argv, '/'))
		++progname;
	else
		progname = *argv;

	if (strstr(progname, "mfs")) {
		mfs = 1;
		Nflag++;
	}

	opstring = mfs ?
	    "D:I:NS:T:a:b:c:d:e:f:g:h:i:m:n:o:s:t:u:" :
	    "NOS:T:U:a:b:c:d:e:f:g:h:i:k:l:m:n:o:p:r:s:t:u:x:";
	while ((ch = getopt(argc, argv, opstring)) != EOF)
		switch (ch) {
		case 'D':	/* to remain undocumented */
			if ((dumpit = creat(dumpf = optarg, 0666)) < 0)
				err(1, dumpf);
			break;
		case 'I':	/* to remain undocumented */
			if ((initit = open(initf = optarg, 0)) < 0)
				err(1, initf);
			break;
		case 'N':
			Nflag = 1;
			break;
		case 'O':
			Oflag = 1;
			break;
		case 'S':
			if ((sectorsize = atoi(optarg)) <= 0)
				fatal("%s: bad sector size", optarg);
			break;
#ifdef COMPAT
		case 'T':
			disktype = optarg;
			break;
#endif
		case 'U':
			if (strcmp(optarg, "soft") == 0)
				dosoftupdates = 1;
			else if (strcmp(optarg, "normal") == 0)
				dosoftupdates = 1;
			else if (strcmp(optarg, "nosoft") == 0)
				dosoftupdates = -1;
			else
				fatal("%s: invalid update mode\n", optarg);
			break;
		case 'a':
			if ((maxcontig = atoi(optarg)) <= 0)
				fatal("%s: bad maximum contiguous blocks\n",
				    optarg);
			break;
		case 'b':
			if ((bsize = atoi(optarg)) < MINBSIZE)
				fatal("%s: bad block size", optarg);
			break;
		case 'c':
			if ((cpg = atoi(optarg)) <= 0)
				fatal("%s: bad cylinders/group", optarg);
			cpgflg++;
			break;
		case 'd':
			if ((rotdelay = atoi(optarg)) < 0)
				fatal("%s: bad rotational delay\n", optarg);
			break;
		case 'e':
			if ((maxbpg = atoi(optarg)) <= 0)
		fatal("%s: bad blocks per file in a cylinder group\n",
				    optarg);
			break;
		case 'f':
			if ((fsize = atoi(optarg)) <= 0)
				fatal("%s: bad fragment size", optarg);
			break;
		case 'g':
			if ((avgfilesize = atoi(optarg)) <= 0)
				fatal("%s: bad average file size", optarg);
			break;
		case 'h':
			if ((avgfilesperdir = atoi(optarg)) <= 0)
				fatal("%s: bad average files per dir", optarg);
			break;
		case 'i':
			if ((density = atoi(optarg)) <= 0)
				fatal("%s: bad bytes per inode\n", optarg);
			break;
		case 'k':
			if ((trackskew = atoi(optarg)) < 0)
				fatal("%s: bad track skew", optarg);
			break;
		case 'l':
			if ((interleave = atoi(optarg)) <= 0)
				fatal("%s: bad interleave", optarg);
			break;
		case 'm':
			if ((minfree = atoi(optarg)) < 0 || minfree > 99)
				fatal("%s: bad free space %%\n", optarg);
			break;
		case 'n':
			if ((nrpos = atoi(optarg)) <= 0)
				fatal("%s: bad rotational layout count\n",
				    optarg);
			break;
		case 'o':
			if (mfs)
				getmntopts(optarg, mopts, &mntflags);
			else {
				if (strcmp(optarg, "space") == 0)
					opt = FS_OPTSPACE;
				else if (strcmp(optarg, "time") == 0)
					opt = FS_OPTTIME;
				else
	fatal("%s: unknown optimization preference: use `space' or `time'.");
			}
			break;
		case 'p':
			if ((trackspares = atoi(optarg)) < 0)
				fatal("%s: bad spare sectors per track",
				    optarg);
			break;
		case 'r':
			if ((rpm = atoi(optarg)) <= 0)
				fatal("%s: bad revolutions/minute\n", optarg);
			break;
		case 's':
			if ((fssize = atoi(optarg)) <= 0)
				fatal("%s: bad file system size", optarg);
			break;
		case 't':
			if ((ntracks = atoi(optarg)) <= 0)
				fatal("%s: bad total tracks", optarg);
			break;
		case 'u':
			if ((nsectors = atoi(optarg)) <= 0)
				fatal("%s: bad sectors/track", optarg);
			break;
		case 'x':
			if ((cylspares = atoi(optarg)) < 0)
				fatal("%s: bad spare sectors per cylinder",
				    optarg);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	switch (argc) {
	case 1:
		specname = mfs ? NULL : argv[0];
		mntname = mfs ? argv[0] : NULL;
		break;
	case 2:
		if (!mfs)
			usage();
		specname = argv[0];
		mntname = argv[1];
		break;
	deafult:
		usage();
	}

	if (specname) {
		special = specname;
		cp = strrchr(special, '/');
		if (cp == 0) {
			/*
			 * No path prefix; try /dev/r%s then /dev/%s.
			 */
			(void)sprintf(device, "%sr%s", _PATH_DEV, special);
			if (stat(device, &st) == -1)
				(void)sprintf(device, "%s%s", _PATH_DEV,
				    special);
			special = device;
		}
		if (Nflag) {
			fso = -1;
		} else {
			fso = open(special, O_WRONLY);
			if (fso < 0)
				fatal("%s: %s", special, strerror(errno));

			/* Bail if target special is mounted */
			n = getmntinfo(&mp, MNT_NOWAIT);
			if (n == 0)
				fatal("%s: getmntinfo: %s", special,
				    strerror(errno));

			len = sizeof(_PATH_DEV) - 1;
			s1 = special;
			if (strncmp(_PATH_DEV, s1, len) == 0)
				s1 += len;

			while (--n >= 0) {
				s2 = mp->f_mntfromname;
				if (strncmp(_PATH_DEV, s2, len) == 0) {
					s2 += len - 1;
					*s2 = 'r';
				}
				if (strcmp(s1, s2) == 0 ||
				    strcmp(s1, &s2[1]) == 0)
					fatal("%s is mounted on %s",
					    special, mp->f_mntonname);
				++mp;
			}
		}
	}
	if (mfs && disktype != NULL) {
		lp = (struct disklabel *)getdiskbyname(disktype);
		if (lp == NULL)
			fatal("%s: unknown disk type", disktype);
		pp = &lp->d_partitions[1];
	} else if (specname) {
		fsi = open(special, O_RDONLY);
		if (fsi < 0)
			fatal("%s: %s", special, strerror(errno));
		if (fstat(fsi, &st) < 0)
			fatal("%s: %s", special, strerror(errno));
		if ((st.st_mode & S_IFMT) != S_IFCHR && !mfs)
			printf("%s: %s: not a character-special device\n",
			    progname, special);
		cp = strchr(specname, '\0') - 1;
		if (cp == (char *)-1 ||
		    (*cp < 'a' || *cp > 'h') && !isdigit(*cp))
			fatal("%s: can't figure out file system partition",
			    specname);
#ifdef COMPAT
		if (!mfs && disktype == NULL)
			disktype = mntname;
#endif
		lp = getdisklabel(special, fsi);
		if (isdigit(*cp))
			pp = &lp->d_partitions[0];
		else
			pp = &lp->d_partitions[*cp - 'a'];
		if (pp->p_size == 0)
			fatal("%s: `%c' partition is unavailable",
			    specname, *cp);
		if (pp->p_fstype == FS_BOOT)
			fatal("%s: `%c' partition overlaps boot program",
			      specname, *cp);
	} else {
		struct disklabel _l;

		if (fssize <= 0)
			fatal("%s: must specify fileystem size", mntname);
		memset(&_l, 0, sizeof(_l));
		_l.d_secsize = 512; 
		_l.d_nsectors = 32; 
		_l.d_ntracks = 16; 
		_l.d_secpercyl = _l.d_nsectors * _l.d_ntracks;
		_l.d_ncylinders = (fssize + _l.d_secpercyl - 1)/_l.d_secpercyl;
		_l.d_secperunit = fssize;
		_l.d_npartitions = MAXPARTITIONS;
		_l.d_partitions[0].p_size = fssize;
		_l.d_partitions[2].p_size = fssize;
		lp = &_l;
		pp = &lp->d_partitions[0];
	}
	if (fssize == 0)
		fssize = pp->p_size;
	if (fssize > pp->p_size && !mfs)
	       fatal("%s: maximum file system size on the `%c' partition is %d",
			specname, *cp, pp->p_size);
	if (rpm == 0) {
		rpm = lp->d_rpm;
		if (rpm <= 0)
			rpm = 3600;
	}
	if (ntracks == 0) {
		ntracks = lp->d_ntracks;
		if (ntracks <= 0)
			fatal("%s: no default #tracks", specname);
	}
	if (nsectors == 0) {
		nsectors = lp->d_nsectors;
		if (nsectors <= 0)
			fatal("%s: no default #sectors/track", specname);
	}
	if (sectorsize == 0) {
		sectorsize = lp->d_secsize;
		if (sectorsize <= 0)
			fatal("%s: no default sector size", specname);
	}
	if (trackskew == -1) {
		trackskew = lp->d_trackskew;
		if (trackskew < 0)
			trackskew = 0;
	}
	if (interleave == 0) {
		interleave = lp->d_interleave;
		if (interleave <= 0)
			interleave = 1;
	}
	if (fsize == 0) {
		fsize = pp->p_fsize;
		if (fsize <= 0)
			fsize = MAX(DFL_FRAGSIZE, lp->d_secsize);
	}
	if (bsize == 0) {
		bsize = pp->p_frag * pp->p_fsize;
		if (bsize <= 0)
			bsize = MIN(DFL_BLKSIZE, 8 * fsize);
	}
	/*
	 * Maxcontig sets the default for the maximum number of blocks
	 * that may be allocated sequentially. With filesystem clustering
	 * it is possible to allocate contiguous blocks up to the maximum
	 * transfer size permitted by the controller or buffering.
	 */
	if (maxcontig == 0)
		maxcontig = MAX(1, MIN(MAXPHYS, MAXBSIZE) / bsize);
	if (density == 0)
		density = NFPI * fsize;
	if (density < fsize)
		/* Absurd values of `density' can lead to overflow errors.  */
		fatal("data bytes per inode (%d) must be >= fragment size (%d)",
		    density, fsize);
	if (minfree < MINFREE && opt != FS_OPTSPACE) {
		fprintf(stderr, "Note: changing optimization to space ");
		fprintf(stderr, "because minfree is less than %d%%\n", MINFREE);
		opt = FS_OPTSPACE;
	}
	if (trackspares == -1) {
		trackspares = lp->d_sparespertrack;
		if (trackspares < 0)
			trackspares = 0;
	}
	nphyssectors = nsectors + trackspares;
	if (cylspares == -1) {
		cylspares = lp->d_sparespercyl;
		if (cylspares < 0)
			cylspares = 0;
	}
	secpercyl = nsectors * ntracks - cylspares;
	if (secpercyl != lp->d_secpercyl)
		fprintf(stderr, "%s (%d) %s (%lu)\n",
			"Note: calculated sectors per cylinder", secpercyl,
			"disagrees with disk label", lp->d_secpercyl);
	if (maxbpg == 0)
		maxbpg = MAXBLKPG(bsize);
	headswitch = lp->d_headswitch;
	trackseek = lp->d_trkseek;
#ifdef notdef /* label may be 0 if faked up by kernel */
	bbsize = lp->d_bbsize;
	sbsize = lp->d_sbsize;
#endif
	oldpartition = *pp;
#ifdef tahoe
	realsectorsize = sectorsize;
	if (sectorsize != DEV_BSIZE) {		/* XXX */
		int secperblk = DEV_BSIZE / sectorsize;

		sectorsize = DEV_BSIZE;
		nsectors /= secperblk;
		nphyssectors /= secperblk;
		secpercyl /= secperblk;
		fssize /= secperblk;
		pp->p_size /= secperblk;
	}
#endif
	mkfs(pp, special, fsi, fso);
#ifdef tahoe
	if (realsectorsize != DEV_BSIZE)
		pp->p_size *= DEV_BSIZE / realsectorsize;
#endif
	if (!Nflag && memcmp(pp, &oldpartition, sizeof(oldpartition)))
		rewritelabel(special, fso, lp);
	if (!Nflag)
		close(fso);
	close(fsi);
#ifdef MFS
	if (mfs) {
		struct mfs_args args;
		sprintf(buf, "mfs:%d", getpid());
		args.fspec = buf;
		args.export.ex_root = -2;
		if (mntflags & MNT_RDONLY)
			args.export.ex_flags = MNT_EXRDONLY;
		else
			args.export.ex_flags = 0;
		args.base = membase;
		args.size = fssize * fsize;
		if (initit >= 0) {
			while (args.size > 0) {
			    	n = read(initit, args.base, args.size);
				if (n <= 0)
					err(1, initf);
				args.base += n;
				args.size -= n;
			}
			close(initit);
			args.size = fssize * fsize;
			args.base = membase;
		}
		if (mount("mfs", mntname, mntflags, &args) < 0)
			fatal("%s: %s", mntname, strerror(errno));
		if (dumpit >= 0) {
			while (args.size > 0) {
				n = write(dumpit, args.base, args.size);
				if (n <= 0)
					err(1, dumpf);
				args.base += n;
				args.size -= n;
			}
			close(dumpit);
		}
	}
#endif
	exit(0);
}

#ifdef COMPAT
char lmsg[] = "%s: can't read disk label; disk type must be specified";
#else
char lmsg[] = "%s: can't read disk label";
#endif

struct disklabel *
getdisklabel(s, fd)
	char *s;
	int fd;
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, (char *)&lab) < 0) {
#ifdef COMPAT
		if (disktype) {
			struct disklabel *lp, *getdiskbyname();

			unlabeled++;
			lp = getdiskbyname(disktype);
			if (lp == NULL)
				fatal("%s: unknown disk type", disktype);
			return (lp);
		}
#endif
		warn("ioctl (GDINFO)");
		fatal(lmsg, s);
	}
	return (&lab);
}

rewritelabel(s, fd, lp)
	char *s;
	int fd;
	register struct disklabel *lp;
{
#ifdef COMPAT
	if (unlabeled)
		return;
#endif
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (ioctl(fd, DIOCWDINFO, (char *)lp) < 0) {
		if (errno == ESRCH)
			warnx("ioctl (WDINFO): no disk label");
		else
			warn("ioctl (WDINFO)");
		fatal("%s: can't rewrite disk label", s);
	}
#if vax
	if (lp->d_type == DTYPE_SMD && lp->d_flags & D_BADSECT) {
		register i;
		int cfd;
		daddr_t alt;
		char specname[64];
		char blk[1024];
		char *cp;

		/*
		 * Make name for 'c' partition.
		 */
		strcpy(specname, s);
		cp = specname + strlen(specname) - 1;
		if (!isdigit(*cp))
			*cp = 'c';
		cfd = open(specname, O_WRONLY);
		if (cfd < 0)
			fatal("%s: %s", specname, strerror(errno));
		memset(blk, 0, sizeof(blk));
		*(struct disklabel *)(blk + LABELOFFSET) = *lp;
		alt = lp->d_ncylinders * lp->d_secpercyl - lp->d_nsectors;
		for (i = 1; i < 11 && i < lp->d_nsectors; i += 2) {
			if (lseek(cfd, (off_t)(alt + i) * lp->d_secsize,
			    L_SET) == -1)
				fatal("lseek to badsector area: %s",
				    strerror(errno));
			if (write(cfd, blk, lp->d_secsize) < lp->d_secsize)
				warn("alternate label %d write", i/2);
		}
		close(cfd);
	}
#endif
}

/*VARARGS*/
void
#if __STDC__
fatal(const char *fmt, ...)
#else
fatal(fmt, va_alist)
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
	if (fcntl(STDERR_FILENO, F_GETFL) < 0) {
		openlog(progname, LOG_CONS, LOG_DAEMON);
		vsyslog(LOG_ERR, fmt, ap);
		closelog();
	} else {
		vwarnx(fmt, ap);
	}
	va_end(ap);
	exit(1);
	/*NOTREACHED*/
}

usage()
{
	if (mfs) {
		fprintf(stderr,
		    "usage: %s [ -fsoptions ] [special-device] mount-point\n",
			progname);
	} else
		fprintf(stderr,
		    "usage: %s [ -fsoptions ] special-device%s\n",
		    progname,
#ifdef COMPAT
		    " [device-type]");
#else
		    "");
#endif
	fprintf(stderr, "where fsoptions are:\n");
	fprintf(stderr,
	    "\t-N do not create file system, just print out parameters\n");
	fprintf(stderr, "\t-O create a 4.3BSD format filesystem\n");
	fprintf(stderr, "\t-S sector size\n");
#ifdef COMPAT
	fprintf(stderr, "\t-T disktype\n");
#endif
	if (!mfs)
		fprintf(stderr, "\t-U type of updates -- `soft' (aka `normal'), or `nosoft')\n");
	fprintf(stderr, "\t-a maximum contiguous blocks\n");
	fprintf(stderr, "\t-b block size\n");
	fprintf(stderr, "\t-c cylinders/group\n");
	fprintf(stderr, "\t-d rotational delay between contiguous blocks\n");
	fprintf(stderr, "\t-e maximum blocks per file in a cylinder group\n");
	fprintf(stderr, "\t-f frag size\n");
	fprintf(stderr, "\t-g average file size\n");
	fprintf(stderr, "\t-h average files per directory\n");
	fprintf(stderr, "\t-i number of bytes per inode\n");
	fprintf(stderr, "\t-k sector 0 skew, per track\n");
	fprintf(stderr, "\t-l hardware sector interleave\n");
	fprintf(stderr, "\t-m minimum free space %%\n");
	fprintf(stderr, "\t-n number of distinguished rotational positions\n");
	fprintf(stderr, "\t-o optimization preference (`space' or `time')\n");
	fprintf(stderr, "\t-p spare sectors per track\n");
	fprintf(stderr, "\t-s file system size (sectors)\n");
	fprintf(stderr, "\t-r revolutions/minute\n");
	fprintf(stderr, "\t-t tracks/cylinder\n");
	fprintf(stderr, "\t-u sectors/track\n");
	fprintf(stderr, "\t-x spare sectors per cylinder\n");
	exit(1);
}