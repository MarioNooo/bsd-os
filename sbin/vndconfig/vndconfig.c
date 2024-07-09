/*
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI vndconfig.c,v 1.6 1998/06/01 17:38:39 torek Exp
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <dev/vndioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void usage __P((void));

static int clearunit __P((int fd, struct vnd_ioctl *cfg));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct vnd_ioctl cfg;
	long unit = 0;
	int force = 0;
	int c, fd, i, cleared;
	char *ep;
	long ctl = VNDIOCSET;
	struct stat s;
	struct statfs fs;
	char devbuf[MAXPATHLEN];
	char *dev;

	cfg.vnd_flags = 0;
	cleared = 0;


	while ((c = getopt(argc, argv, "cfsu:")) != EOF)
	switch (c) {
	case 'c':
		ctl = VNDIOCCLR;
		break;
	case 'f':
		force = 1;
		break;
	case 's':
		cfg.vnd_flags |= VND_F_SWAP;
		break;
	case 'u':
		unit = strtol(optarg, &ep, 10);
		if ((unit == LONG_MIN ||
		    unit == LONG_MAX) && errno == ERANGE)
			err(1, "%s", optarg);
		if (optarg[0] == '\0' || ep[0] != '\0')
			errx(1, "%s: Invalid numeric argument", optarg);
		if (unit < 0)
			errx(1, "%s: Invalid unit number", optarg);
		break;
	case '?':
	default:
		usage();
	}
	argc -= optind;
	argv += optind;



	if (ctl == VNDIOCCLR) {
		if (argc != 0)
			usage();
	} else {
		if (argc != 1)
			usage();
		if ((dev = realpath(argv[0], devbuf)) == NULL)
			errx(1, "error in pathname \"%s\"", devbuf);
		cfg.vnd_file = dev;
	}
	cfg.vnd_unit = unit;
	fd = open("/dev/vnd", O_RDWR);
	if (fd < 0)
		err(1, "/dev/vnd");

	if (ctl == VNDIOCCLR) 
		exit (clearunit(fd, &cfg));

	if (strlen(dev) >= VND_SAVESIZE - 1) {
		warnx("device path name \"%s\" too long", dev);
		errx(1, "max device path length %d", VND_SAVESIZE - 1);
	}

	if (stat(dev, &s))
		err(1, dev);

	if (statfs(dev, &fs))	/* force the stat info into kernel */
		err(1, dev);

	if (!(S_ISREG(s.st_mode)))
		errx(1, "%s: is not a regular file");

	if ((s.st_flags & UF_IMMUTABLE) && force) {
		warnx("\"%s\" immutable bit previously set; clearing", dev);
		if (chflags(dev, s.st_flags & ~UF_IMMUTABLE))
			err(1, "unable to clear immutable bit on %s", dev);
		cleared = 1;
	}
	if ((s.st_flags & UF_IMMUTABLE) && !force) {
		warnx("immutable bit set on \"%s\"", dev);
		errx(1, "use -f to force clear");
	}
	if (s.st_uid != 0)
		warnx("warning \"%s\" not owned by root", dev);

	if (s.st_mode & (S_IRGRP | S_IROTH))
		warnx("warning \"%s\" readable by group and/or other", dev);

	strcpy((char*)cfg.vnd_save, dev);
	if (ioctl(fd, VNDIOCSET, &cfg) < 0) {
		warn("kernel rejected vnd unit %d", unit);
		if (cleared) {
			warnx("resetting immutable bit");
			if (chflags(dev, s.st_flags | UF_IMMUTABLE))
				err(1, "unable to set immutable bit on \"%s\"",
				    dev);
		}
		exit(1);
	}

	if (chflags(dev, s.st_flags | UF_IMMUTABLE))
		warn("warning unable to set immutable bit on \"%s\"", dev);
	exit (0);
}

static int
clearunit(fd, cfg)
	int fd;
	struct vnd_ioctl *cfg;
{
	struct stat s;
	char *dev;

	if (ioctl(fd, VNDIOCCLR, cfg) < 0) 
		err(1, "%d: kernel rejected clearing vnd unit", cfg->vnd_unit);

	dev = (char*)cfg->vnd_save;

	if (stat(dev, &s))
		err(1, "warning immutable bit not cleared \"%\"s", dev);

	if (chflags(dev, s.st_flags & ~UF_IMMUTABLE)) 
		errx(1, "warning unable to clear immutable bit on \"%s\"", dev);

	return (0);
}

void
usage()
{
	(void)fprintf(stderr,
"usage: vndconfig: [-u unit] filename\n");
	(void)fprintf(stderr,
"       vndconfig: -c [-u unit] \n");
	exit (1);
}
