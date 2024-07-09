/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI panasonic.c,v 2.4 1997/09/10 19:07:56 sanders Exp
 */

/* libcdrom driver for Panasonic CR-5XX rev 1.0c */

#include <cdrom.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <dev/scsi/scsi.h>
#include <dev/scsi/scsi_ioctl.h>

#include "devs.h"

struct cdinfo *
panasonic_probe(fd, vendor, product, rev, vers)
	int fd;
	char *vendor;
	char *product;
	char *rev;
	int vers;
{
	int i;
	int val;
	u_char buf[1000];
	u_char cdb[16];
	int first_track, last_track;
	int ntracks;
	int track_num;
	struct track_info *tp;
	struct cdinfo *cdinfo;
	u_char *raw;

	if (strcmp (vendor, "MATSHITA") != 0)
		return (NULL);

	/* get first and last track numbers */
	bzero(cdb, 10);
	cdb[0] = 0xc3;
	cdb[1] = 2; /* msf format */
	cdb[7] = sizeof buf >> 8;
	cdb[8] = sizeof buf & 0xff;
	if (scsi_cmd(fd, cdb, 10, buf, 4)) {
		perror("scsi command for read toc");
		return (NULL);
	}

	first_track = buf[2];
	last_track = buf[3];
	ntracks = last_track - first_track + 1;

	if ((cdinfo = make_cdinfo(fd, ntracks)) == NULL)
		return (NULL);
	
	for (track_num = first_track, tp = cdinfo->tracks, raw = buf + 4;
	     track_num <= last_track;
	     track_num++, tp++, raw += 8) {
		tp->track_num = track_num;
		
		tp->start_frame = msf_to_frame(raw[5], raw[6], raw[7]);
		tp->control = raw[1] & 0xff;
	}

	cdinfo->total_frames = msf_to_frame(raw[5], raw[6], raw[7]);
	
	return (cdinfo);
}

int
panasonic_close(cdinfo)
struct cdinfo *cdinfo;
{
	close(cdinfo->fd);
	free(cdinfo->tracks);
	free(cdinfo);
	return (0);
}

int
panasonic_play(cdinfo, start_frame, end_frame)
struct cdinfo *cdinfo;
int start_frame;
int end_frame;

{
	u_char cdb[16];
	u_char buf[100];
	int minute, second, frame;

	frame_to_msf(start_frame, &minute, &second, &frame);

	bzero(cdb, 10);
	cdb[0] = 0xc7;
	cdb[3] = minute;
	cdb[4] = second;
	cdb[5] = frame;

	frame_to_msf(end_frame, &minute, &second, &frame);
	cdb[6] = minute;
	cdb[7] = second;
	cdb[8] = frame;

	if (scsi_cmd(cdinfo->fd, cdb, 10, buf, 1)) {
		perror("scsi command: audio search");
		return (-1);
	}

	return (0);
}

int
panasonic_stop(cdinfo)
struct cdinfo *cdinfo;
{
	u_char cdb[10];

	bzero(cdb, 10);
	cdb[0] = 0xcb;

	if (scsi_cmd(cdinfo->fd, cdb, 10, NULL, 0) < 0) {
		perror("scsi command for stop");
		return (-1);
	}
	return (0);
}

int
panasonic_status(cdinfo, cdstatus)
struct cdinfo *cdinfo;
struct cdstatus *cdstatus;
{
	u_char cdb[10];
	u_char buf[50];

	bzero(cdb, 10);
	cdb[0] = 0xc2;
	cdb[1] = 2;
	cdb[2] = 0x40;
	cdb[3] = 1; /* get current position */
	cdb[7] = sizeof buf >> 8;
	cdb[8] = sizeof buf;

	if (scsi_cmd_read(cdinfo->fd, cdb, 10, buf, sizeof buf) !=
	    sizeof buf) {
		perror("scsi command get status");
		return (-1);
	}

	switch (buf[1]) {
	case 0x11:
		cdstatus->state = cdstate_playing;
		break;
	case 0x12:
		cdstatus->state = cdstate_paused;
		break;
	case 0x13:
	case 0x14:
		cdstatus->state = cdstate_stopped;
		break;
	default:
		cdstatus->state = cdstate_unknown;
		break;
	}

	cdstatus->control = buf[5] & 0xff;
	cdstatus->track_num = buf[6];
	cdstatus->index_num = buf[7];

	cdstatus->rel_frame = msf_to_frame(buf[13], buf[14], buf[15]);
	cdstatus->abs_frame = msf_to_frame(buf[9], buf[10], buf[11]);
	
	return (0);
}

int
panasonic_eject(cdinfo)
struct cdinfo *cdinfo;
{
	return (0);
}
	
int
panasonic_volume(cdinfo, volume)
struct cdinfo *cdinfo;
int volume;
{
	return (0);
}

int
panasonic_pause(cdinfo, pause_resume)
	struct cdinfo *cdinfo;
	int pause_resume;
{
	fprintf(stderr, "pause/resume not implmented for panasonic CD\n");
	return (0);
}
