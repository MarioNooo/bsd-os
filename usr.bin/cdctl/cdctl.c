/*-
 * Copyright (c) 1992,1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI cdctl.c,v 2.9 2000/04/07 19:41:31 jch Exp
 */

/*
 * This program provides a simple way to play audio from the cdrom,
 * as well as display the table of contents.
 */
#include <sys/cdefs.h>

#include <cdrom.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_toc __P((struct cdinfo *));
void print_toc_hash __P((struct cdinfo *));
void usage __P((void));

int hflag;
int jflag;
int lflag;
int pflag;
int Sflag;
int tflag;
int vflag;
int wflag;
int Pflag;	/* previous track */
int Nflag;	/* next track */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	struct cdinfo *cdinfo;
	struct cdstatus status;
	struct track_info *stp, *etp, *tp;
	int c, i, end_track, start_track;
	int minute, second, frame;
	char *e, *fname;
	uid_t uid;
	int volume;

	if ((fname = getenv("CDROM")) == NULL)
		fname = _PATH_CDROM;
	start_track = end_track = volume = -1;
	while ((c = getopt(argc, argv, "e:f:hjlNPpSs:tvwV:")) != EOF) {
		switch (c) {
		case 'e':
			end_track = strtol(optarg, &e, 10);
			if (end_track < 0 || *optarg == '\0' || *e != '\0')
				errx(1, "illegal interface -- %s", optarg);
			break;
		case 'f':
			fname = optarg;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'j':
			jflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'N':
			Nflag = 1;
			break;
		case 'P':
			Pflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'S':
			Sflag = 1;
			break;
		case 's':
			start_track = strtol(optarg, &e, 10);
			if (start_track < 0 || *optarg == '\0' || *e != '\0')
				errx(1, "illegal interface -- %s", optarg);
			break;
		case 't':
			tflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		case 'V':
			volume = strtol(optarg, &e, 10);
			if (volume < 0 || volume > 100
			    || *optarg == '\0' || *e != '\0')
				errx(1, "illegal interface -- %s", optarg);
			break;
		default:
			usage();
		}
	}

	/* Device must be accessible by regular user */
	if (access(fname, R_OK) < 0)
		err(1, "%s", fname);

	/* Try twice in case a disk was just inserted. */
	cdinfo = cdopen(fname);
	if (cdinfo == NULL)
		cdinfo = cdopen(fname);
	if (cdinfo == NULL)
		errx(1, "cdopen of %s failed", fname);

#if 0
	/* Switch back to regular user */
	/* Not for 3.0 - we need to be root to do the ioctl's too */
	uid = getuid();
	if (uid != 0 && setuid(getuid()) < 0)
		err(1, "setuid");
#endif

	if (volume >= 0)
		cdvolume(cdinfo, volume);

	if (hflag)
		print_toc_hash(cdinfo);

	if (lflag || jflag) {
		if (lflag && jflag)
			errx(1, "The -l and -j options are mutually exclusive");
		if (jflag)
			cdeject(cdinfo);
		if (lflag)
			cdload(cdinfo);
	}

	if (Sflag)
		cdstop(cdinfo);

	if (tflag)
		print_toc(cdinfo);

	/*
	 * If we don't have any flags that indicate that the user wants
	 * to play the cdrom, and we've already done something, quit.
	 */
	if (start_track == -1 && end_track == -1 && !pflag && !wflag &&
	    (hflag || lflag || jflag || Sflag || tflag))
		exit (0);

	stp = NULL;
	etp = NULL;

	/* process -N and -P flags */
	if (Nflag && Pflag)
		errx(1, "The -N and -P options are mutually exclusive");
	if (cdstatus(cdinfo, &status) < 0)
		errx(1, "cdstatus failed");
	if ((Nflag || Pflag) && status.state != cdstate_playing)
		errx(1, "cd isn't currently playing");
	if (Nflag)
		start_track = status.track_num + 1;
	if (Pflag)
		start_track = status.track_num - 1;

	if (start_track == -1)
		stp = cdinfo->tracks;
	if (end_track == -1)
		etp = &cdinfo->tracks[cdinfo->ntracks - 1];

	for (i = 0, tp = cdinfo->tracks; i < cdinfo->ntracks; i++, tp++) {
		if (tp->track_num == start_track)
			stp = tp;
		if (tp->track_num == end_track)
			etp = tp;
	}

	if (stp == NULL)
		errx(1, "can't find start track %d", start_track);
	if (etp == NULL)
		errx(1, "can't find ending track %d", end_track);

	cdplay(cdinfo, stp->start_frame, etp->start_frame + etp->nframes);

	if (pflag == 0 && wflag == 0)
		exit(0);

	for (;;) {
		if (cdstatus(cdinfo, &status) < 0)
			errx(1, "cdstatus failed");
		
		if (status.state != cdstate_playing)
			break;

		if (pflag) {
			printf("track %d ", status.track_num);
			printf("index %d ", status.index_num);

			frame_to_msf(status.rel_frame,
			    &minute, &second, &frame);
			printf("rel %02d:%02d:%02d ", minute, second, frame);

			frame_to_msf(status.abs_frame,
			    &minute, &second, &frame);
			printf("abs %02d:%02d:%02d ", minute, second, frame);
			printf("\n");
		}
		sleep(1);
	}
	exit (0);
}

void
print_toc(cdinfo)
	struct cdinfo *cdinfo;
{
	struct track_info *tp;
	int i, minute, second, frame;

	for (i = 0, tp = cdinfo->tracks; i < cdinfo->ntracks; i++, tp++) {
		frame_to_msf(tp->nframes, &minute, &second, &frame);
		printf("%2d %02d:%02d:%02d",
		    tp->track_num, minute, second, frame);
		if (vflag)
			printf(" 0x%02x", tp->control);
		printf("\n");
	}
}

void
print_toc_hash(cdinfo)
	struct cdinfo *cdinfo;
{
	struct track_info *tp;
	struct track_hash_info {
		int	track_num;
		int	nframes;
		int	start_frame;
	} *track_hash_info, *thp;
	int i, track_hash_size;

	track_hash_size = cdinfo->ntracks * sizeof (struct track_hash_info);
	if ((track_hash_info = calloc(1, track_hash_size)) == NULL)
		err(1, NULL);

	for (i = 0, tp = cdinfo->tracks,
	    thp = track_hash_info; i < cdinfo->ntracks; i++, tp++, thp++) {
		thp->track_num = tp->track_num;
		thp->nframes = tp->nframes;
		thp->start_frame = tp->start_frame;
	}

	(void)printf("%d\n", crc_buf(track_hash_info, track_hash_size));
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: cdctl [-Shjlptvw] [-e track] [-f file] [-s track]\n");
	exit(1);
}
