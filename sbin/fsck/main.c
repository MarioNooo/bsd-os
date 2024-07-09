/*
 * Copyright (c) 1980, 1986, 1993
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
static const char copyright[] =
"@(#) Copyright (c) 1980, 1986, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static const char sccsid[] = "@(#)main.c	8.6 (Berkeley) 5/14/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/sysctl.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ffs/fs.h>

#include <err.h>
#include <errno.h>
#include <fstab.h>
#include <paths.h>
#include <string.h>

#include "fsck.h"

static void usage __P((void));
static int argtoi __P((int flag, char *req, char *str, int base));
static int docheck __P((struct fstab *fsp));
static int checkfilesys __P((char *filesys, char *mntpt, long auxdata,
		int child));
static struct statfs *getmntpt __P((const char *));
int main __P((int argc, char *argv[]));

int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int ch;
	int ret = 0, maxrun = 0;
	struct rlimit rlimit;

	sync();
	skipclean = 1;
	while ((ch = getopt(argc, argv, "b:Bc:dfFl:m:pnNyY")) != -1) {
		switch (ch) {
		case 'b':
			bflag = argtoi('b', "number", optarg, 10);
			printf("Alternate super block location: %d\n", bflag);
			break;

		case 'B':
			bkgrdflag = 1;
			break;

		case 'c':
			skipclean = 0;
			cvtlevel = argtoi('c', "conversion level", optarg, 10);
			break;

		case 'd':
			debug++;
			break;

		case 'f':
			skipclean = 0;
			break;

		case 'F':
			bkgrdcheck = 1;
			break;

		case 'l':
			maxrun = argtoi('l', "number", optarg, 10);
			break;

		case 'm':
			lfmode = argtoi('m', "mode", optarg, 8);
			if (lfmode &~ 07777)
				errx(EEXIT, "bad mode to -m: %o", lfmode);
			printf("** lost+found creation mode %o\n", lfmode);
			break;

		case 'n':
		case 'N':
			nflag++;
			yflag = 0;
			break;

		case 'p':
			preen++;
			break;

		case 'y':
		case 'Y':
			yflag++;
			nflag = 0;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, catch);
	if (preen)
		(void)signal(SIGQUIT, catchquit);
	signal(SIGINFO, infohandler);
	/*
	 * Push up our allowed memory limit so we can cope
	 * with huge filesystems.
	 */
	if (getrlimit(RLIMIT_DATA, &rlimit) == 0) {
		rlimit.rlim_cur = rlimit.rlim_max;
		(void)setrlimit(RLIMIT_DATA, &rlimit);
	}
	if (argc) {
		while (argc-- > 0) {
			char *path = blockcheck(*argv);

			if (path == NULL)
				pfatal("Can't check %s\n", *argv);
			else
				(void)checkfilesys(path, 0, 0L, 0);
			++argv;
		}
		exit(0);
	}
	ret = checkfstab(preen, maxrun, docheck, checkfilesys);
	if (returntosingle)
		exit(2);
	exit(ret);
}

static int
argtoi(flag, req, str, base)
	int flag;
	char *req, *str;
	int base;
{
	char *cp;
	int ret;

	ret = (int)strtol(str, &cp, base);
	if (cp == str || *cp)
		errx(EEXIT, "-%c flag requires a %s", flag, req);
	return (ret);
}

/*
 * Determine whether a filesystem should be checked.
 */
static int
docheck(fsp)
	register struct fstab *fsp;
{

	if (strcmp(fsp->fs_vfstype, "ufs") ||
	    (strcmp(fsp->fs_type, FSTAB_RW) &&
	     strcmp(fsp->fs_type, FSTAB_RO)) ||
	    fsp->fs_passno == 0)
		return (0);
	return (1);
}

/*
 * Check the specified filesystem.
 */
/* ARGSUSED */
static int
checkfilesys(filesys, mntpt, auxdata, child)
	char *filesys, *mntpt;
	long auxdata;
	int child;
{
	ufs_daddr_t n_ffree, n_bfree;
	struct ufs_args args;
	struct dups *dp;
	struct statfs *mntp;
	struct zlncnt *zlnp;
	ufs_daddr_t blks;
	ufs_daddr_t files;
	int cylno, size;
	int flags;

	if (preen && child)
		(void)signal(SIGQUIT, voidquit);
	cdevname = filesys;
	if (debug && preen)
		pwarn("starting\n");
	/*
	 * Make best effort to get the disk name. Check first to see
	 * if it is listed among the mounted filesystems. Failing that
	 * check to see if it is listed in /etc/fstab.
	 */
	mntp = getmntpt(filesys);

	/*
	 * XXX not yet: mntp->f_mntfromname is always the block device,
	 * while in BSD/OS, we want the raw name for all but the mounted
	 * root device.  Also, getmntpt() above currently returns NULL
	 * for (e.g.) "fsck /" (presumably because / is rwd0a, not wd0a).
	 */
#ifdef notyet
	if (mnpt != NULL)
		filesys = mntp->f_mntfromname;
	else
		filesys = blockcheck(filesys);
#endif
	/*
	 * If -F flag specified, check to see whether a background check
	 * is possible and needed. If possible and needed, exit with
	 * status zero. Otherwise exit with status non-zero. A non-zero
	 * exit status will cause a foreground check to be run.
	 */
	sblock_init();
	if (bkgrdcheck) {
		if ((fsreadfd = open(filesys, O_RDONLY)) < 0 || readsb(0) == 0)
			exit(3);	/* Cannot read superblock */
		close(fsreadfd);
		if (sblock.fs_flags & FS_NEEDSFSCK)
			exit(4);	/* Earlier background failed */
		if ((sblock.fs_flags & FS_DOSOFTDEP) == 0)
			exit(5);	/* Not running soft updates */
		size = MIBSIZE;
		if (sysctlnametomib("vfs.ffs.adjrefcnt", adjrefcnt, &size) < 0)
			exit(6);	/* Lacks kernel support */
		if ((mntp == NULL && sblock.fs_clean == 1) ||
		    (mntp != NULL && (sblock.fs_flags & FS_UNCLEAN) == 0))
			exit(7);	/* Filesystem clean, report it now */
		exit(0);
	}
	/*
	 * If we are to do a background check:
	 *	Get the mount point information of the filesystem
	 *	create snapshot file
	 *	return created snapshot file
	 *	if not found, clear bkgrdflag and proceed with normal fsck
	 */
	if (bkgrdflag) {
		if (mntp == NULL) {
			bkgrdflag = 0;
			pfatal("NOT MOUNTED, CANNOT RUN IN BACKGROUND\n");
		} else if ((mntp->f_flags & MNT_SOFTDEP) == 0) {
			bkgrdflag = 0;
			pfatal("NOT USING SOFT UPDATES, %s\n",
			    "CANNOT RUN IN BACKGROUND");
		} else if ((mntp->f_flags & MNT_RDONLY) != 0) {
			bkgrdflag = 0;
			pfatal("MOUNTED READ-ONLY, CANNOT RUN IN BACKGROUND\n");
		} else if ((fsreadfd = open(filesys, O_RDONLY)) >= 0) {
			if (readsb(0) != 0) {
				if (sblock.fs_flags & FS_NEEDSFSCK) {
					bkgrdflag = 0;
					pfatal("UNEXPECTED INCONSISTENCY, %s\n",
					    "CANNOT RUN IN BACKGROUND\n");
				}
				if ((sblock.fs_flags & FS_UNCLEAN) == 0 &&
				    skipclean && preen) {
					/*
					 * filesystem is clean;
					 * skip snapshot and report it clean
					 */
					pwarn("FILESYSTEM CLEAN; %s\n",
					    "SKIPPING CHECKS");
					goto clean;
				}
			}
			close(fsreadfd);
		}
		if (bkgrdflag) {
			snprintf(snapname, sizeof snapname, "%s/.fsck_snapshot",
			    mntp->f_mntonname);
			args.fspec = snapname;
			while (mount("ffs", mntp->f_mntonname,
			    mntp->f_flags | MNT_UPDATE | MNT_SNAPSHOT,
			    &args) < 0) {
				if (errno == EEXIST && unlink(snapname) == 0)
					continue;
				bkgrdflag = 0;
				pfatal("CANNOT CREATE SNAPSHOT %s: %s\n",
				    snapname, strerror(errno));
				break;
			}
			if (bkgrdflag != 0)
				filesys = snapname;
		}
	}

	switch (setup(filesys)) {
	case 0:
		if (preen)
			pfatal("CAN'T CHECK FILE SYSTEM.");
		/* fall through */
	case -1:
	clean:
#if 0 /* not sure why FreeBSD bothers to mention all this */
		pwarn("clean, %ld free ", (long)(sblock.fs_cstotal.cs_nffree +
		    sblock.fs_frag * sblock.fs_cstotal.cs_nbfree));
		printf("(%d frags, %d blocks, %.1f%% fragmentation)\n",
		    sblock.fs_cstotal.cs_nffree, sblock.fs_cstotal.cs_nbfree,
		    sblock.fs_cstotal.cs_nffree * 100.0 / sblock.fs_dsize);
#endif
		return (0);
	}
	/*
	 * Cleared if any questions answered no. Used to decide if
	 * the superblock should be marked clean.
	 */
	resolved = 1;
	/*
	 * 1: scan inodes tallying blocks used
	 */
	if (preen == 0) {
		printf("** Last Mounted on %s\n", sblock.fs_fsmnt);
#ifdef notyet
		if (mntp != NULL && mntp->f_flags & MNT_ROOTFS)
#else
		if (hotroot)
#endif
			printf("** Root file system\n");
		printf("** Phase 1 - Check Blocks and Sizes\n");
	}
	pass1();

	/*
	 * 1b: locate first references to duplicates, if any
	 */
	if (duplist) {
		if (preen || usedsoftdep)
			pfatal("INTERNAL ERROR: dups with -p");
		printf("** Phase 1b - Rescan For More DUPS\n");
		pass1b();
	}

	/*
	 * 2: traverse directories from root to mark all connected directories
	 */
	if (preen == 0)
		printf("** Phase 2 - Check Pathnames\n");
	pass2();

	/*
	 * 3: scan inodes looking for disconnected directories
	 */
	if (preen == 0)
		printf("** Phase 3 - Check Connectivity\n");
	pass3();

	/*
	 * 4: scan inodes looking for disconnected files; check reference counts
	 */
	if (preen == 0)
		printf("** Phase 4 - Check Reference Counts\n");
	pass4();

	/*
	 * 5: check and repair resource counts in cylinder groups
	 */
	if (preen == 0)
		printf("** Phase 5 - Check Cyl groups\n");
	pass5();

	/*
	 * print out summary statistics
	 */
	n_ffree = sblock.fs_cstotal.cs_nffree;
	n_bfree = sblock.fs_cstotal.cs_nbfree;
	files = maxino - ROOTINO - sblock.fs_cstotal.cs_nifree - n_files;
	blks = n_blks +
	    sblock.fs_ncg * (cgdmin(&sblock, 0) - cgsblock(&sblock, 0));
	blks += cgsblock(&sblock, 0) - cgbase(&sblock, 0);
	blks += howmany(sblock.fs_cssize, sblock.fs_fsize);
	blks = maxfsblock - (n_ffree + sblock.fs_frag * n_bfree) - blks;
	if (bkgrdflag && (files > 0 || blks > 0)) {
		countdirs = sblock.fs_cstotal.cs_ndir - countdirs;
		pwarn("Reclaimed: %ld directories, %ld files, %d fragments\n",
		    countdirs, (long)files - countdirs, blks);
	}
	pwarn("%ld files, %ld used, %ld free ",
	    (long)n_files, (long)n_blks, (long)(n_ffree +
	     sblock.fs_frag * n_bfree));
	printf("(%d frags, %d blocks, %.1f%% fragmentation)\n",
	    n_ffree, n_bfree, n_ffree * 100.0 / sblock.fs_dsize);
	if (debug) {
		if (files < 0)
			printf("%d files missing\n", -files);
		if (blks < 0)
			printf("%d blocks missing\n", -blks);
		if (duplist != NULL) {
			printf("The following duplicate blocks remain:");
			for (dp = duplist; dp; dp = dp->next)
				printf(" %d,", dp->dup);
			printf("\n");
		}
		if (zlnhead != NULL) {
			printf("The following zero link count inodes remain:");
			for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
				printf(" %u,", zlnp->zlncnt);
			printf("\n");
		}
	}
	zlnhead = (struct zlncnt *)0;
	duplist = (struct dups *)0;
	muldup = (struct dups *)0;
	inocleanup();
	if (fsmodified) {
		sblock.fs_time = time(NULL);
		sbdirty();
	}
	if (cvtlevel && sblk.b_dirty) {
		/*
		 * Write out the duplicate super blocks
		 */
		for (cylno = 0; cylno < sblock.fs_ncg; cylno++)
			bwrite(fswritefd, (char *)&sblock,
			    fsbtodb(&sblock, cgsblock(&sblock, cylno)), SBSIZE);
	}
	if (rerun)
		resolved = 0;

	/*
	 * Check to see if the filesystem is mounted read-write.
	 */
	flags = 0;
#ifdef notyet
	if (bkgrdflag == 0 && mntp != NULL) {
		flags = mntp->f_flags;
		if ((mntp->f_flags & MNT_RDONLY) == 0)
			resolved = 0;
	}
#else
	/* ugh, this is wrong (but close enough for now?) */
	if (hotroot) {
		struct statfs stfs_buf;
		/*
		 * Check to see if root is mounted read-write.
		 */
		if (statfs("/", &stfs_buf) == 0)
			flags = stfs_buf.f_flags;
		if ((flags & MNT_RDONLY) == 0)
			resolved = 0;
	}
#endif
	ckfini(resolved);

	for (cylno = 0; cylno < sblock.fs_ncg; cylno++)
		if (inostathead[cylno].il_stat != NULL)
			free((char *)inostathead[cylno].il_stat);
	free((char *)inostathead);
	inostathead = NULL;
	if (fsmodified && !preen)
		printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
	if (rerun)
		printf("\n***** PLEASE RERUN FSCK *****\n");
	if (hotroot) {
		struct ufs_args args;
		int ret;
		/*
		 * We modified the root.  Do a mount update on
		 * it, unless it is read-write, so we can continue.
		 */
		if (flags & MNT_RDONLY) {
			args.fspec = 0;
			args.export.ex_flags = 0;
			args.export.ex_root = 0;
			flags |= MNT_UPDATE | MNT_RELOAD;
			ret = mount("ufs", "/", flags, &args);
			if (ret == 0)
				return (0);
		}
		if (!fsmodified)
			return (0);
		if (!preen)
			printf("\n***** REBOOT NOW *****\n");
		sync();
		return (4);
	}
	return (0);
}

/*
 * Get the mount point information for name.
 */
static struct statfs *
getmntpt(name)
	const char *name;
{
	struct stat devstat, mntdevstat;
	char *devname;
	struct statfs *mntbuf, *statfsp;
	char device[sizeof statfsp->f_mntfromname];
	int i, mntsize, isdev;

	if (stat(name, &devstat) != 0)
		return (NULL);
	if (S_ISCHR(devstat.st_mode) || S_ISBLK(devstat.st_mode))
		isdev = 1;
	else
		isdev = 0;
	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = 0; i < mntsize; i++) {
		statfsp = &mntbuf[i];
		devname = statfsp->f_mntfromname;
		if (*devname != '/') {
			snprintf(device, sizeof device, "%s%s",
			    _PATH_DEV, devname);
			strcpy(statfsp->f_mntfromname, device);
		}
		if (isdev == 0) {
			if (strcmp(name, statfsp->f_mntonname))
				continue;
			return (statfsp);
		}
		if (stat(devname, &mntdevstat) == 0 &&
		    mntdevstat.st_rdev == devstat.st_rdev)
			return (statfsp);
	}
	statfsp = NULL;
	return (statfsp);
}

static void
usage()
{
	extern char *__progname;

	(void) fprintf(stderr,
	    "Usage: %s [-BFpfny] [-b block] [-c level] [-m mode] "
			"filesystem ..\n",
	    __progname);
	exit(1);
}
