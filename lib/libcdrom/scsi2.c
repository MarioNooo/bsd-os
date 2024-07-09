/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI scsi2.c,v 2.6 1997/09/10 19:07:56 sanders Exp
 */

/*
 * This file contains the device specific code for the SCSI-2 audio commands.
 */

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

/* These are the extra scsi commands we need. */

static int scsi2_version;

struct cdinfo *
scsi2_probe(fd, vendor, product, rev, vers)
	int fd;
	char *vendor;
	char *product;
	char *rev;
	int vers;
{
	int i;
	u_char buf[100];
	u_char cdb[10];
	int first_track, last_track;
	int ntracks;
	int track_num;
	struct track_info *tp;
	struct cdinfo *cdinfo;
	int scsi_toc_size;
	u_char *scsi_toc;
	u_char *p;

	if (*vendor == 0)
		return (NULL); /* not a scsi drive */

	/* check for SCSI-2 or ATAPI */
	if (vers < 2 && vers != 0)
		return (NULL);

	scsi2_version = vers;


	bzero(cdb, 10);
	cdb[0] = 0x43;
	cdb[1] = 2;
	cdb[8] = 4;
	if (scsi_cmd(fd, cdb, 10, buf, 4) < 0)
		/* drive doesn't support scsi2 or no disk */
		return (NULL);

	first_track = buf[2] & 0xff;
	last_track = buf[3] & 0xff;
	ntracks = last_track - first_track + 1;
	
	scsi_toc_size = 4 + (ntracks + 1) * 8;
	if ((scsi_toc = malloc(scsi_toc_size)) == NULL)
		return (NULL);

	bzero(cdb, 10);
	cdb[0] = 0x43;
	cdb[1] = 2; /* msf format */
	cdb[6] = first_track;
	cdb[7] = scsi_toc_size >> 8;
	cdb[8] = scsi_toc_size;
	if (scsi_cmd(fd, cdb, 10, scsi_toc, scsi_toc_size) < 0) {
		perror("scsi command for toc data");
		return (NULL);
	}
	
	if ((cdinfo = make_cdinfo(fd, ntracks)) == NULL)
		return (NULL);

	for (track_num = first_track, tp = cdinfo->tracks, p = scsi_toc + 4;
	     track_num <= last_track; 
	     track_num++, tp++, p += 8) {
		tp->track_num = track_num;
		tp->start_frame = msf_to_frame(p[5] & 0xff, p[6] & 0xff,
		    p[7] & 0xff);
		tp->control = p[1] & 0xff;
	}

	cdinfo->total_frames = msf_to_frame(p[5] & 0xff, p[6] & 0xff,
	    p[7] & 0xff);

	scsi2_volume(cdinfo, 100);

	return (cdinfo);
}

int
scsi2_close(cdinfo)
	struct cdinfo *cdinfo;
{
	close(cdinfo->fd);
	free(cdinfo->tracks);
	free(cdinfo);
	return (0);
}

int
scsi2_play(cdinfo, start_frame, end_frame)
	struct cdinfo *cdinfo;
	int start_frame;
	int end_frame;
{
	int minute, second, frame;
	u_char cdb[10];

	bzero(cdb, sizeof(cdb));
	cdb[0] = 0x47;

	frame_to_msf(start_frame, &minute, &second, &frame);
	cdb[3] = minute;
	cdb[4] = second;
	cdb[5] = frame;

	frame_to_msf(end_frame - 1, &minute, &second, &frame);
	cdb[6] = minute;
	cdb[7] = second;
	cdb[8] = frame;

	if (scsi_cmd(cdinfo->fd, cdb, sizeof(cdb), NULL, 0) < 0) {
		perror("scsi command for play");
		return (-1);
	}

	return (0);
}

int
scsi2_stop(cdinfo)
	struct cdinfo *cdinfo;
{
	u_char cdb[6];

	bzero(cdb, sizeof(cdb));
	cdb[0] = 0x1b;
	cdb[1] = 1;
	cdb[4] = 0;	/* lower bits: eject start/stop */
	if (scsi_cmd(cdinfo->fd, cdb, sizeof(cdb), NULL, 0) < 0) {
		perror("scsi command for stop");
		return (-1);
	}
	return (0);
}

int
scsi2_status(cdinfo, cdstatus)
	struct cdinfo *cdinfo;
	struct cdstatus *cdstatus;
{
	u_char cdb[10];
	u_char buf[16];

	bzero(cdb, sizeof(cdb));
	cdb[0] = 0x42;
	cdb[1] = 2; /* msf format */
	cdb[2] = 0x40; /* request q channel */
	cdb[3] = 1; /* current position */
	cdb[7] = sizeof buf >> 8;
	cdb[8] = sizeof buf;

	if (scsi_cmd(cdinfo->fd, cdb, sizeof(cdb), buf, sizeof buf) < 0) {
		perror("scsi command for status");
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
	case 0x15:
	default:
		cdstatus->state = cdstate_stopped;
		break;
	}

	cdstatus->control = buf[5] & 0xff;
	cdstatus->track_num = buf[6] & 0xff;
	cdstatus->index_num = buf[7] & 0xff;
	cdstatus->abs_frame = msf_to_frame(buf[9] & 0xff, buf[10] & 0xff,
	    buf[11] & 0xff);
	cdstatus->rel_frame = msf_to_frame(buf[13] & 0xff, buf[14] & 0xff,
	    buf[15] & 0xff);

	return (0);
}

int
scsi2_eject(cdinfo)
	struct cdinfo *cdinfo;
{
	u_char cdb[6];

	bzero(cdb, sizeof(cdb));
	cdb[0] = 0x1b;
	cdb[1] = 1;
	cdb[4] = 2;
	if (scsi_cmd(cdinfo->fd, cdb, sizeof(cdb), NULL, 0) < 0) {
		perror("scsi command for eject");
		return (-1);
	}
	return (0);
}
	
int
scsi2_load(cdinfo)
	struct cdinfo *cdinfo;
{
	u_char cdb[6];

	bzero(cdb, sizeof(cdb));
	cdb[0] = 0x1b;
	cdb[1] = 1;
	cdb[4] = 3;
	if (scsi_cmd(cdinfo->fd, cdb, sizeof(cdb), NULL, 0) < 0) {
		perror("scsi command for load");
		return (-1);
	}
	return (0);
}
	
/* volume is 0 .. 100 */
int
scsi2_volume(cdinfo, volume)
	struct cdinfo *cdinfo;
	int volume;
{
	unsigned char cdb[12];
	unsigned char buf[256];
	int val;
	int rc;
	int i;

	val = (volume * 255) / 100;
	if (val < 0)
		val = 0;
	if (val > 255)
		val = 255;

	/* mode sense (6/10) */
	bzero(cdb, sizeof(cdb));
	if (scsi2_version == 0) {
		cdb[0] = 0x5a;
		cdb[8] = 254;
		i = 10;
	} else {
		cdb[0] = 0x1a;
		cdb[4] = 254;
		i = 6;
	}
	cdb[2] = 0xe;
	if ((rc = scsi_cmd_read(cdinfo->fd, cdb, i, buf, 254)) < 0)
		return (-1);

	/* Build mode select */

	/* Adjust for mode paramter header (10) or (6) and block descriptors */
	if (scsi2_version == 0) {
		rc = buf[1] + 2;	/* Length + length field itself */
		buf[1] = 0;
		i = 8 + buf[3] + 8;	/* ATAPI MPH is 8 bytes */
	} else {
		rc = buf[0] + 1;
		buf[0] = 0;
		i = 4 + buf[3] + 8;
	}

	/* Fill in the volume */
	buf[i] = 1;
	buf[i+1] = val;
	buf[i+2] = 2;
	buf[i+3] = val;

	/* build the CDB */
	bzero(cdb, sizeof(cdb));
	if (scsi2_version == 0) {
		cdb[0] = 0x55;
		cdb[8] = rc;
		i = 10;
	} else {
		cdb[0] = 0x15;
		cdb[4] = rc;
		i = 6;
	}
	cdb[1] = 0x10;

	if (scsi_cmd_write(cdinfo->fd, cdb, i, buf, rc) < 0)
		return (-1);

	return (0);
}

int
scsi2_pause(cdinfo, pause_resume)
	struct cdinfo *cdinfo;
	int pause_resume;
{
	u_char cdb[10];

	bzero(cdb, sizeof(cdb));
	cdb[0] = 0x4b;		/* PAUSE/RESUME */
	cdb[8] = pause_resume & 0x1;
	if (scsi_cmd(cdinfo->fd, cdb, sizeof(cdb), NULL, 0) < 0) {
		perror("scsi command for pause/resume");
		return (-1);
	}

	return (0);
}
