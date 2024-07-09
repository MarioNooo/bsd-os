/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI toshiba.c,v 2.4 1997/09/10 19:07:56 sanders Exp
 */

/* libcdrom driver for Toshiba 3201B SCSI CD-ROM drive. */

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
toshiba_probe(fd, vendor, product, rev, vers)
	int fd;
	char *vendor;
	char *product;
	char *rev;
	int vers;
{
	int i;
	int val;
	u_char buf[100];
	u_char cdb[16];
	int first_track, last_track;
	int ntracks;
	int track_num;
	struct track_info *tp;
	struct cdinfo *cdinfo;

	if (strcmp (vendor, "TOSHIBA") != 0)
		return (NULL);

	/* get first and last track numbers */

	bzero(cdb, 10);
	cdb[0] = 0xc7;
	if (scsi_cmd(fd, cdb, 10, buf, 4)) {
		perror("scsi command for first/last track num");
		return (NULL);
	}


	first_track = bcd_to_int(buf[0]);
	last_track = bcd_to_int(buf[1]);
	ntracks = last_track - first_track + 1;

	if ((cdinfo = make_cdinfo(fd, ntracks)) == NULL)
		return (NULL);
	
	/* get end time */
	bzero(cdb, 10);
	cdb[0] = 0xc7;
	cdb[1] = 1;
		
	if (scsi_cmd(fd, cdb, 10, buf, 4)) {
		perror("scsi command for get end time");
		return (NULL);
	}
	
	cdinfo->total_frames = msf_to_frame(bcd_to_int(buf[0]),
	    bcd_to_int(buf[1]), bcd_to_int(buf[2]));
	
	for (track_num = first_track, tp = cdinfo->tracks;
	     track_num <= last_track;
	     track_num++, tp++) {
		tp->track_num = track_num;
		
		bzero(cdb, 10);
		cdb[0] = 0xc7;
		cdb[1] = 2;
		cdb[2] = int_to_bcd(track_num);
		
		if (scsi_cmd(fd, cdb, 10, buf, 4)) {
			perror("scsi command for get track info");
			return (NULL);
		}

		tp->start_frame = msf_to_frame(bcd_to_int(buf[0]),
		    bcd_to_int(buf[1]), bcd_to_int(buf[2]));
		tp->control = buf[3] & 0xff;
	}

	return (cdinfo);
}

int
toshiba_close(cdinfo)
struct cdinfo *cdinfo;
{
	close(cdinfo->fd);
	free(cdinfo->tracks);
	free(cdinfo);
	return (0);
}

int
toshiba_play(cdinfo, start_frame, end_frame)
struct cdinfo *cdinfo;
int start_frame;
int end_frame;

{
	u_char cdb[16];
	u_char buf[100];
	int minute, second, frame;

	frame_to_msf(start_frame, &minute, &second, &frame);

	bzero(cdb, 10);
	cdb[0] = 0xc0;
	cdb[1] = 0; /* seek, but don't start playing yet */
	cdb[2] = int_to_bcd(minute);
	cdb[3] = int_to_bcd(second);
	cdb[4] = int_to_bcd(frame);
	cdb[9] = 0x40;

	if (scsi_cmd(cdinfo->fd, cdb, 10, buf, 1)) {
		perror("scsi command: audio search");
		return (-1);
	}

	frame_to_msf(end_frame, &minute, &second, &frame);

	bzero(cdb, 10);
	cdb[0] = 0xc1;
	cdb[1] = 3; /* stereo */
	cdb[2] = int_to_bcd(minute);
	cdb[3] = int_to_bcd(second);
	cdb[4] = int_to_bcd(frame);
	cdb[9] = 0x40;

	if (scsi_cmd(cdinfo->fd, cdb, 10, buf, 1)) {
		perror("scsi command: play audio");
		return (-1);
	}

	return (0);
}

int
toshiba_stop(cdinfo)
struct cdinfo *cdinfo;
{
	u_char cdb[10];

	bzero(cdb, 10);
	cdb[0] = 0xc2;

	if (scsi_cmd(cdinfo->fd, cdb, 10, NULL, 0) < 0) {
		perror("scsi command for stop");
		return (-1);
	}
	return (0);
}

int
toshiba_status(cdinfo, cdstatus)
struct cdinfo *cdinfo;
struct cdstatus *cdstatus;
{
	u_char cdb[10];
	u_char buf[10];

	bzero(cdb, 10);
	cdb[0] = 0xc6;
	cdb[1] = sizeof buf;

	if (scsi_cmd_read(cdinfo->fd, cdb, 10, buf, sizeof buf) != 
	    sizeof buf) {
		perror("scsi command get status");
		return (-1);
	}

	switch (buf[0]) {
	case 0:
		cdstatus->state = cdstate_playing;
		break;
	case 1:
	case 2:
		cdstatus->state = cdstate_paused;
		break;
	case 3:
		cdstatus->state = cdstate_stopped;
		break;
	default:
		cdstatus->state = cdstate_unknown;
		break;
	}

	cdstatus->control = buf[1] & 0xff;
	cdstatus->track_num = bcd_to_int(buf[2]);
	cdstatus->index_num = bcd_to_int(buf[3]);

	cdstatus->rel_frame = msf_to_frame(bcd_to_int(buf[4]),
	    bcd_to_int(buf[5]), bcd_to_int(buf[6]));
	cdstatus->abs_frame = msf_to_frame(bcd_to_int(buf[7]),
	    bcd_to_int(buf[8]), bcd_to_int(buf[9]));
	
	return (0);
}

int
toshiba_eject(cdinfo)
struct cdinfo *cdinfo;
{
	u_char cdb[10];

	bzero(cdb, 10);
	cdb[0] = 0xc4;
	cdb[1] = 1;

	if (scsi_cmd(cdinfo->fd, cdb, 10, NULL, 0) < 0) {
		perror("scsi command for eject");
		return (-1);
	}
	return (0);
}
	
int
toshiba_volume(cdinfo, volume)
struct cdinfo *cdinfo;
int volume;
{
	/* Not supported by drive */
	return (0);
}

int
toshiba_pause(cdinfo, pause_resume)
	struct cdinfo *cdinfo;
	int pause_resume;
{
	fprintf(stderr, "pause/resume not implmented for toshiba CD\n");
	return (0);
}
