/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI fdformat.c,v 2.5 1997/10/13 21:29:44 prb Exp
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>

#include <err.h>
#include <fcntl.h>
#include <paths.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FDBLK			512
#define	MAX_SECPERTRACK		18

struct fmt {
	char cyl;
	char h;
	char sec;
	char type;
};

int vflag;		/* verbose */

void format __P((int, struct disklabel *));
void genfid __P((struct fmt *, struct disklabel *, int, int));
void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	struct disklabel *dp;
	struct stat sb;
	int ch, fd, lflag;
	char *dkname, *specname, *type;
	char cmdbuf[MAXPATHLEN + 100], namebuf[MAXPATHLEN];

	lflag = 0;
	while ((ch = getopt(argc, argv, "vl")) != EOF)
	switch (ch) {
	case 'l':		/* put disklabel to disk after formatting */
		lflag = 1;
		break;
	case 'v':		/* verbose */
		vflag = 1;
		break;
	case '?':
	default:
		usage();
	}
	argc -= optind;
	argv += optind;

	dkname = argv[0];
	switch (argc) {
	case 1:
		type = "floppy";
		break;
	case 2:
		type = argv[1];
		break;
	default:
		usage();
		/* NOTREACHED */
	}

	if (dkname[0] != '/') {
		(void)snprintf(namebuf,
		    sizeof(namebuf), "%sr%s", _PATH_DEV, dkname);
		fd = open(namebuf, O_WRONLY, 0);
		if (fd < 0) {
			(void)snprintf(namebuf,
			    sizeof(namebuf), "%sr%sa", _PATH_DEV, dkname);
			fd = open(namebuf, O_WRONLY, 0);
		}
		if (fd < 0) {
			(void)snprintf(namebuf,
			    sizeof(namebuf), "%s%s", _PATH_DEV, dkname);
			fd = open(namebuf, O_WRONLY, 0);
		}
		if (fd < 0) {
			(void)snprintf(namebuf,
			    sizeof(namebuf), "%s%sa", _PATH_DEV, dkname);
			fd = open(namebuf, O_WRONLY, 0);
		}
		if (fd >= 0)
			specname = dkname;
		else
			specname = namebuf;
	} else {
		specname = dkname;
		fd = open(specname, O_WRONLY, 0);
	}

	if (fd < 0)
		err(1, "%s", specname);

	dp = getdiskbyname(type);
	if (dp == NULL)
		errx(1, "unknown disk type: %s\n", type);

	/*
	 * Set kernel idea of disk geometry for specified format.
	 */
	dp->d_checksum = 0;
	dp->d_checksum = dkcksum(dp);
	if (ioctl(fd, DIOCSDINFO, (char *)dp))
		err(1, "%s: can't set disk label", specname);

	if (vflag) {
		printf("Formatting %s as %s: ", specname, type);
		fflush(stdout);
	}
	format(fd, dp);
	if (close(fd))
		err(1, "%s", specname);

	if (vflag)
		printf("\tFormatting done.\n");
	if (lflag) {
		snprintf(cmdbuf, sizeof(cmdbuf),
		     "disksetup -w %s %s", specname, type);
		if (vflag)
			printf("%s\n", cmdbuf);
		if (system(cmdbuf))
			err(1, "disksetup failed");
	}
	exit(0);
}

void
format(fd, dp)
	int fd;
	register struct disklabel *dp;
{
	struct format_op df;
	struct fmt fid[MAX_SECPERTRACK];
	int cyl, h;

	df.df_buf = (char *)fid;
	df.df_startblk = 0;
	df.df_count = sizeof(struct fmt) * dp->d_nsectors;

	if (vflag) {
		printf("track          ");	/* 1 + 9 spaces - see below*/
		fflush(stdout);
	}
	for (cyl = 0; cyl < dp->d_ncylinders; ++cyl)
		for (h = 0; h < dp->d_ntracks; ++h) {
			genfid(fid, dp, cyl, h);
			if (ioctl(fd, DIOCWFORMAT, &df) < 0) {
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
				fflush(stdout);
				err(1, NULL);
			}
			if (vflag) {
				/* 9 \b - see above :-) */
				printf("\b\b\b\b\b\b\b\b\b%2d head %1d",
				    cyl, h);
				fflush(stdout);
			}
			df.df_startblk +=
			    dp->d_nsectors * dp->d_secsize / FDBLK;
		}
}

/*
 * genfid --
 *	generate format id array for given track and head
 */
void
genfid(fid, dp, cyl, h)
	register struct fmt *fid;
	register struct disklabel *dp;
	int cyl, h;
{
	int i, sec, il;
	char type;

	il = dp->d_interleave;
	if (il == 0)			/* ??? */
		il = 1;			/* ??? */
	if ((unsigned)il >= dp->d_nsectors)
		errx(1, "interleave (%d) > sectors-per-track (%d)",
		    il, dp->d_nsectors);

	for (i = 0 ; i < dp->d_nsectors; i++ )
		fid[i].sec = 0;

	type = dp->d_secsize >> 8;
	if (type == 4)
		type = 3;

	for (i = 0, sec = 1; sec <= dp->d_nsectors ; sec++) {
		fid[i].cyl = (char) cyl;
		fid[i].h = (char) h;
		fid[i].sec = (char) sec;
		fid[i].type = type;

		i += il;
		if (i >= dp->d_nsectors) {
			i -= dp->d_nsectors;
			/*
			 * The loop below runs iff nsectors % il == 0
			 * (and it runs at most nsectors/2 times).
			 */
			while (fid[i].sec != 0)
				i++;
		}
	}
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: fdformat [-lv] device disktab_type\n");
	exit(1);
}
