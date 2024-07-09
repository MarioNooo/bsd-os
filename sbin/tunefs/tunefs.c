/*
 * Copyright (c) 1983, 1993
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
static char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)tunefs.c	8.3 (Berkeley) 5/3/95";
#endif /* not lint */

/*
 * tunefs: change layout parameters to an existing file system.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ffs/fs.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdio.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* the optimization warning string template */
#define	OPTWARN	"should optimize for %s with minfree %s %d%%"

union {
	struct	fs sb;
	char pad[MAXBSIZE];
} sbun;
#define	sblock sbun.sb

int fi, hotroot;
long dev_bsize = 1;

void bwrite(daddr_t, char *, int);
int bread(daddr_t, char *, int);
char *blockcheck(char *);
char *rawname(char *);
char *unrawname(char *);
void getsb(struct fs *, char *);
void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *cp, *special, *name, *action;
	struct stat st;
	int i, flags;
	int Aflag = 0, Nflag = 0;
	struct statfs stfs_buf;
	char *chg[2], device[MAXPATHLEN];
	struct ufs_args args;
	struct fstab *fs;

	argc--, argv++; 
	if (argc < 2)
		usage();
	special = argv[argc - 1];
	fs = getfsfile(special);
	if (fs)
		special = fs->fs_spec;
	special = blockcheck(special);
	if (hotroot) {
		/*
		 * Check to see if root is mounted read-write.
		 */
		if (statfs("/", &stfs_buf) == 0)
			flags = stfs_buf.f_flags;
		if ((flags & MNT_RDONLY) == 0)
			errx(8, "Tuning the root filesystem requires %s\n",
			    "that it be mounted read-only");
	}
	getsb(&sblock, rawname(special));
	chg[FS_OPTSPACE] = "space";
	chg[FS_OPTTIME] = "time";
	for (; argc > 0 && argv[0][0] == '-'; argc--, argv++) {
		for (cp = &argv[0][1]; *cp; cp++)
			switch (*cp) {

			case 'A':
				Aflag++;
				continue;

			case 'N':
				Nflag++;
				continue;

			case 'a':
				name = "maximum contiguous block count";
				if (argc < 1)
					errx(10, "-a: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 1)
					errx(10, "%s must be >= 1 (was %s)",
					    name, *argv);
				warnx("%s changes from %d to %d",
				    name, sblock.fs_maxcontig, i);
				sblock.fs_maxcontig = i;
				continue;

			case 'd':
				name =
				   "rotational delay between contiguous blocks";
				if (argc < 1)
					errx(10, "-d: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				warnx("%s changes from %dms to %dms",
				    name, sblock.fs_rotdelay, i);
				sblock.fs_rotdelay = i;
				continue;

			case 'e':
				name =
				  "maximum blocks per file in a cylinder group";
				if (argc < 1)
					errx(10, "-e: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 1)
					errx(10, "%s must be >= 1 (was %s)",
					    name, *argv);
				warnx("%s changes from %d to %d",
				    name, sblock.fs_maxbpg, i);
				sblock.fs_maxbpg = i;
				continue;

			case 'f':
				name = "average file size";
				if (argc < 1)
					errx(10, "-f: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 1)
					errx(10, "%s must be >= 1 (was %s)",
					    name, *argv);
				warnx("%s changes from %d to %d",
				    name, sblock.fs_avgfilesize, i);
				sblock.fs_avgfilesize = i;
				continue;

			case 'm':
				name = "minimum percentage of free space";
				if (argc < 1)
					errx(10, "-m: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 0 || i > 99)
					errx(10, "bad %s (%s)", name, *argv);
				warnx("%s changes from %d%% to %d%%",
				    name, sblock.fs_minfree, i);
				sblock.fs_minfree = i;
				if (i >= MINFREE &&
				    sblock.fs_optim == FS_OPTSPACE)
					warnx(OPTWARN, "time", ">=", MINFREE);
				if (i < MINFREE &&
				    sblock.fs_optim == FS_OPTTIME)
					warnx(OPTWARN, "space", "<", MINFREE);
				continue;

			case 'o':
				name = "optimization preference";
				if (argc < 1)
					errx(10, "-o: missing %s", name);
				argc--, argv++;
				if (strcmp(*argv, chg[FS_OPTSPACE]) == 0)
					i = FS_OPTSPACE;
				else if (strcmp(*argv, chg[FS_OPTTIME]) == 0)
					i = FS_OPTTIME;
				else
					errx(10, "bad %s (options are `space' or `time')",
					    name);
				if (sblock.fs_optim == i) {
					warnx("%s remains unchanged as %s",
					    name, chg[i]);
					continue;
				}
				warnx("%s changes from %s to %s",
				    name, chg[sblock.fs_optim], chg[i]);
				sblock.fs_optim = i;
				if (sblock.fs_minfree >= MINFREE &&
				    i == FS_OPTSPACE)
					warnx(OPTWARN, "time", ">=", MINFREE);
				if (sblock.fs_minfree < MINFREE &&
				    i == FS_OPTTIME)
					warnx(OPTWARN, "space", "<", MINFREE);
				continue;

			case 'n':
				name = "soft updates";
				if (argc < 1)
					errx(10, "-s: missing %s", name);
				argc--, argv++;

				/* Soft updates make no sense for ramdisk */
				if (fs != NULL && 
				    ((strcmp(fs->fs_spec, "/dev/rd") == 0) ||
				    (strcmp(fs->fs_spec, "/dev/rrd") == 0))) {
					warnx("%s cannot be modified for "
					    "ramdisk", name);
					continue;
				}

				if (strcmp(*argv, "enable") == 0) {
					sblock.fs_flags |= FS_DOSOFTDEP;
					action = "set";
				} else if (strcmp(*argv, "disable") == 0) {
					sblock.fs_flags &= ~FS_DOSOFTDEP;
					action = "cleared";
				} else {
					errx(10, "bad %s (options are %s)",
					    name, "`enable' or `disable'");
				}
				warnx("%s %s", name, action);
				continue;

			case 's':
				name = "expected number of files per directory";
				if (argc < 1)
					errx(10, "-s: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 1)
					errx(10, "%s must be >= 1 (was %s)",
					    name, *argv);
				warnx("%s changes from %d to %d",
				    name, sblock.fs_avgfpdir, i);
				sblock.fs_avgfpdir = i;
				continue;

			case 't':
				name = "track skew in sectors";
				if (argc < 1)
					errx(10, "-t: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 0)
					errx(10, "%s: %s must be >= 0",
						*argv, name);
				warnx("%s changes from %d to %d",
					name, sblock.fs_trackskew, i);
				sblock.fs_trackskew = i;
				continue;

			default:
				usage();
			}
	}
	if (argc != 1)
		usage();
	if (Nflag) {
		fprintf(stdout, "tunefs: current settings\n");
		fprintf(stdout, "\tmaximum contiguous block count %d\n",
		    sblock.fs_maxcontig);
		fprintf(stdout, "\taverage file size %d\n",
		    sblock.fs_avgfilesize);
		fprintf(stdout, "\taverage number of files per directory %d\n",
		    sblock.fs_avgfpdir);
		fprintf(stdout,
		    "\trotational delay between contiguous blocks %dms\n",
		    sblock.fs_rotdelay);
		fprintf(stdout,
		    "\tmaximum blocks per file in a cylinder group %d\n",
		    sblock.fs_maxbpg);
		fprintf(stdout, "\tminimum percentage of free space %d%%\n",
		    sblock.fs_minfree);
		fprintf(stdout, "\toptimization preference: %s\n",
		    chg[sblock.fs_optim]);
		fprintf(stdout, "\tsoft updates: %s\n",
		    (sblock.fs_flags & FS_DOSOFTDEP) ? "enabled" : "disabled");
		fprintf(stdout, "\ttrack skew %d sectors\n",
			sblock.fs_trackskew);
		fprintf(stdout, "tunefs: no changes made\n");
		exit(0);
	}
	fi = open(special, 1);
	if (fi < 0) {
		fprintf(stdout, "tunefs: no changes made\n");
		err(3, "cannot open %s for writing", special);
	}
	bwrite((daddr_t)SBOFF / dev_bsize, (char *)&sblock, SBSIZE);
	if (Aflag)
		for (i = 0; i < sblock.fs_ncg; i++)
			bwrite(fsbtodb(&sblock, cgsblock(&sblock, i)),
			    (char *)&sblock, SBSIZE);
	close(fi);
	/*
	 * If we modified the root, do a mount update on it.
	 */
	if (hotroot) {
		args.fspec = 0;
		args.export.ex_flags = 0;
		args.export.ex_root = 0;
		flags |= MNT_UPDATE | MNT_RELOAD;
		if (mount("ffs", "/", flags, &args) < 0)
			err(9, "mount update failed");
	}
	exit(0);
}

void
usage()
{

	fprintf(stderr, "Usage: tunefs [-AN] tuneup-options special-device\n");
	fprintf(stderr, "where tuneup-options are:\n");
	fprintf(stderr, "\t-d rotational delay between contiguous blocks\n");
	fprintf(stderr, "\t-a maximum contiguous blocks\n");
	fprintf(stderr, "\t-e maximum blocks per file in a cylinder group\n");
	fprintf(stderr, "\t-f average file size\n");
	fprintf(stderr, "\t-m minimum percentage of free space\n");
	fprintf(stderr, "\t-o optimization preference (`space' or `time')\n");
	fprintf(stderr, "\t-n soft updates (`enable' or `disable')\n");
	fprintf(stderr, "\t-s expected number of files per directory\n");
	fprintf(stderr, "\t-t track skew in sectors\n");
	exit(2);
}

void
getsb(fs, file)
	register struct fs *fs;
	char *file;
{

	fi = open(file, 0);
	if (fi < 0)
		err(3, "cannot open %s for reading", file);
	if (bread((daddr_t)SBOFF, (char *)fs, SBSIZE))
		err(4, "%s: bad super block", file);
	if (fs->fs_magic != FS_MAGIC)
		err(5, "%s: bad magic number", file);
	dev_bsize = fs->fs_fsize / fsbtodb(fs, 1);
	close(fi);
}

void
bwrite(blk, buf, size)
	daddr_t blk;
	char *buf;
	int size;
{

	if (lseek(fi, (off_t)blk * dev_bsize, SEEK_SET) < 0)
		err(6, "FS SEEK");
	if (write(fi, buf, size) != size)
		err(7, "FS WRITE");
}

int
bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
	int cnt;
{
	int i;

	if (lseek(fi, (off_t)bno * dev_bsize, SEEK_SET) < 0)
		return(1);
	if ((i = read(fi, buf, cnt)) != cnt) {
		for(i=0; i<sblock.fs_bsize; i++)
			buf[i] = 0;
		return (1);
	}
	return (0);
}

char *
blockcheck(origname)
	char *origname;
{
	struct stat stslash, stblock, stchar;
	char *newname, *raw;
	int retried = 0;

	hotroot = 0;
	if (stat("/", &stslash) < 0) {
		printf("Can't stat /: %s\n", strerror(errno));
		return (origname);
	}
	newname = origname;
retry:
	if (stat(newname, &stblock) < 0) {
		printf("Can't stat %s: %s\n", newname, strerror(errno));
		return (origname);
	}
	if ((stblock.st_mode & S_IFMT) == S_IFBLK) {
		if (stslash.st_dev == stblock.st_rdev)
			hotroot++;
		else
			return (newname);
		raw = rawname(newname);
		if (stat(raw, &stchar) < 0) {
			printf("Can't stat %s: %s\n", raw, strerror(errno));
			return (origname);
		}
		if ((stchar.st_mode & S_IFMT) == S_IFCHR) {
			return (raw);
		} else {
			printf("%s is not a character device\n", raw);
			return (origname);
		}
	} else if ((stblock.st_mode & S_IFMT) == S_IFCHR && !retried) {
		newname = unrawname(newname);
		retried++;
		goto retry;
	}
	/*
	 * Not a block or character device, just return name and
	 * let the user decide whether to use it.
	 */
	return (origname);
}

char *
unrawname(name)
	char *name;
{
	char *dp;
	struct stat stb;

	if ((dp = strrchr(name, '/')) == 0)
		return (name);
	if (stat(name, &stb) < 0)
		return (name);
	if ((stb.st_mode & S_IFMT) != S_IFCHR)
		return (name);
	if (dp[1] != 'r')
		return (name);
	(void)strcpy(&dp[1], &dp[2]);
	return (name);
}

char *
rawname(name)
	char *name;
{
	static char rawbuf[32];
	char *dp;

	if ((dp = strrchr(name, '/')) == 0)
		return (0);
	if (dp[1] == 'r' || name == rawbuf)
		return (name);
	*dp = 0;
	(void)strcpy(rawbuf, name);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, &dp[1]);
	return (rawbuf);
}
