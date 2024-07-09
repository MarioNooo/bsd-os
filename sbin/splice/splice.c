/*
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI splice.c,v 1.4 1996/06/07 22:39:06 cp Exp
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <dev/spioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct spliceinfo {
	char	*s_name;
	dev_t	 s_dev;
	u_int	 s_size;
} spliceinfo_t; 

#define SP_SKIP	32		/* leave first 32 sectors empty */
#define DEFAULT_INTERLACE 2048	/* 1 meg */

splice_config_t *buildstripe __P((spliceinfo_t*, int, int));
splice_config_t *buildconcatenation __P((spliceinfo_t*, int));
void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	splice_config_t *cfg;
	spliceinfo_t *devs, *d;
	long interlace, unit;
	int c, devcnt, fd, i, rpm, verbose;
	char *ep;

	unit = 0;
	rpm = verbose = 0;
	interlace = DEFAULT_INTERLACE;
	while ((c = getopt(argc, argv, "i:u:v")) != EOF)
	switch (c) {
	case 'i':
		interlace = strtol(optarg, &ep, 10);
		if ((interlace == LONG_MIN ||
		    interlace == LONG_MAX) && errno == ERANGE)
			err(1, "%s", optarg);
		if (optarg[0] == '\0' || ep[0] != '\0')
			errx(1, "%s: Invalid numeric argument", optarg);
		if (interlace < 0)
			errx(1, "%s: Invalid interleave value", optarg);
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
	case 'v':
		verbose = 1;
		break;
	case '?':
	default:
		usage();
	}
	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	if ((devs = malloc(sizeof(*devs) * argc)) == NULL)
		errx(1, NULL);
	memset(devs, 0, sizeof(*devs) * argc);
	d = devs;

	for (devcnt = argc; argc--; ++d, ++argv) {
		struct disklabel l;
		struct partition *p;
		struct stat s;
		char *x;

		fd = open(*argv, O_RDONLY);
		if (fd < 0)
			err(1, "%s", *argv);
		if (fstat(fd, &s) < 0)
			err(1, "%s", *argv);
		if (!S_ISCHR(s.st_mode) && !S_ISBLK(s.st_mode))
			errx(1, "%s: neither character or block special device",
			    *argv);
		if (ioctl(fd, DIOCGDINFO, (char *)&l) < 0)
			errx(1, "%s: unable to get disk label", *argv);
		x = argv[0] + strlen(argv[0]) - 1;
		if (*x < 'a' || *x > 'h')
			errx(1, "%s: unable to compute partition", *argv);
		p = &l.d_partitions[*x - 'a'];
		if (p->p_size == 0)
			errx(1, "%s: has size of zero", *argv);
		d->s_name = *argv;
		d->s_dev = s.st_rdev;
		d->s_size = p->p_size;
		if (l.d_rpm > rpm)
			rpm = l.d_rpm;
		if (interlace != 0) {
			if (d->s_size < (interlace + SP_SKIP))
				errx(1, "%s: too small, minimum %d",
				    *argv, interlace + SP_SKIP);
		} else {
			if (d->s_size <= (interlace + SP_SKIP))
				errx(1, "%s: too small, minimum %d",
				    *argv, interlace + SP_SKIP);
		}
		(void)close(fd);
	}
	if ((d = malloc(sizeof(*devs) * devcnt)) == NULL)
		err(1, NULL);
	memmove(d, devs, sizeof(*devs) * devcnt);

	if (interlace != 0) 
		cfg = buildstripe(d, devcnt, interlace);
	else
		cfg = buildconcatenation(d, devcnt);

	if (verbose) {
		splice_row_t *row;

		printf("width     first   last       offset devs\n");
		for (row = cfg->c_row; row != NULL; row = row->r_next) {
			printf("%5d%10d%10d%10d",
			    row->r_devcount, row->r_firstblk,
			    row->r_lastblk, row->r_offset);
			for (i = 0; i < row->r_devcount; i++) {
				int j;
				char *p;

				for (j = 0; j < devcnt; j++)
					if (row->r_devs[i] == devs[j].s_dev)
						break;
				p = devs[j].s_name;
				if (strncmp(p, "/dev/", 5) == 0)
					p += 5;
				printf(" %s", p);
			}
			printf("\n");
		}
	}

	fd = open("/dev/splice", O_RDWR);
	if (fd < 0)
		err(1, "/dev/splice");

	cfg->c_unit = unit;
	cfg->c_rpm = rpm;
	if (ioctl(fd, SPSETSPLICE, cfg) < 0) 
		err(1, "%d: kernel rejected splice unit", unit);

	exit (0);
}

splice_config_t *
buildstripe(devs, devcnt, interlace)
	spliceinfo_t *devs;
	int devcnt, interlace;
{
	static splice_config_t config;
	daddr_t firstblk, offset;
	splice_row_t *r, **rp;
	u_int smallest;
	int cyclecnt, i, j;

	firstblk = 0;
	offset = 32;
	rp = &config.c_row;

	for (i = 0; i < devcnt; i++)
		devs[i].s_size -= SP_SKIP;

	while (devcnt) {
		if ((r = malloc(sizeof(*r))) == NULL)
			err(1, NULL);
		*rp = r;
		rp = &r->r_next;
		r->r_next = NULL;
		r->r_firstblk = firstblk;
		r->r_devcount = devcnt;
		r->r_offset = offset;
		if ((r->r_devs = malloc (sizeof(dev_t) * devcnt)) == NULL)
			err(1, NULL);

		/*
		 * Find the smallest device.
		 */
		smallest = UINT_MAX;
		for (i = 0; i < devcnt; i++) {
			r->r_devs[i] = devs[i].s_dev; 
			if (devs[i].s_size < smallest)
				smallest = devs[i].s_size;
		}
		cyclecnt = smallest / interlace;
		r->r_interlace = interlace;
		r->r_cyclesize = interlace * devcnt;
		firstblk += (cyclecnt * interlace * devcnt);
		r->r_lastblk = firstblk - 1;
		offset += (cyclecnt * interlace);
		for (i = 0; i < devcnt; i++) {
			devs[i].s_size -= cyclecnt * interlace;
			if (devs[i].s_size < interlace) {
				for (j = i; j < devcnt - 1; j++)
					devs[j] = devs[j + 1];
				i--;
				devcnt--;
			}
		}
	}
	return (&config);
}

splice_config_t *
buildconcatenation(devs, devcnt)
	spliceinfo_t *devs;
	int devcnt;
{
	static splice_config_t config;
	daddr_t firstblk;
	splice_row_t *r, **rp;
	int cyclecnt, i;

	firstblk = 0;
	rp = &config.c_row;

	for (i = 0; i < devcnt; i++) {
		devs[i].s_size -= SP_SKIP;

		if ((r = malloc(sizeof(*r))) == NULL)
			err(1, NULL);
		*rp = r;
		rp = &r->r_next;
		r->r_next = NULL;
		r->r_firstblk = firstblk;
		r->r_devcount = 1;
		r->r_offset = SP_SKIP;
		if ((r->r_devs = malloc(sizeof(dev_t))) == NULL)
			err(1, NULL);

		r->r_devs[0] = devs[i].s_dev; 
		cyclecnt = 1;
		r->r_interlace = devs[i].s_size;
		r->r_cyclesize = devs[i].s_size;
		firstblk += devs[i].s_size;
		r->r_lastblk = firstblk - 1;
	}
	return (&config);
}

void
usage()
{
	(void)fprintf(stderr,
"usage: splice: [-v] [-i interleave] [-u unit] part part [part ...]\n");
	exit (1);
}
