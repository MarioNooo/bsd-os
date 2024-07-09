/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI common.c,v 2.6 1998/11/13 14:27:29 torek Exp
 */

#include <cdrom.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <dev/scsi/scsi.h>
#include <dev/scsi/scsi_ioctl.h>

#include "devs.h"

struct cdfuncs cdfuncs[] = {
{
	"scsi2",
	scsi2_probe,
	scsi2_close,
	scsi2_play,
	scsi2_stop,
	scsi2_status,
	scsi2_eject,
	scsi2_volume,
	scsi2_load,
	scsi2_pause,
},
{
	"panasonic",
	panasonic_probe,
	panasonic_close,
	panasonic_play,
	panasonic_stop,
	panasonic_status,
	panasonic_eject,
	panasonic_volume,
	panasonic_eject,
	panasonic_pause,
},
{
	"toshiba",
	toshiba_probe,
	toshiba_close,
	toshiba_play,
	toshiba_stop,
	toshiba_status,
	toshiba_eject,
	toshiba_volume,
	toshiba_eject,
	toshiba_pause,
},
#ifdef MITSUMI
{
	"mitsumi",
	mitsumi_probe,
	mitsumi_close,
	mitsumi_play,
	mitsumi_stop,
	mitsumi_status,
	mitsumi_eject,
	mitsumi_volume,
	mitsumi_eject,
	mitsumi_pause,
},
#endif
{ NULL },
};

/*
 * Open the cdrom special device, determine the type of drive, and
 * return a handle that includes the table of contents.
 */
struct cdinfo *
cdopen(fname)
	char *fname;
{
	int fd;
	struct cdinfo *cdinfo;
	int i;
	struct track_info *tp;
	int val;
	char vendor[17];
	char product[33];
	char rev[5];
	int vers;

	if (fname == NULL)
		fname = getenv("CDROM");

	if (fname == NULL || *fname == 0)
		fname = _PATH_CDROM;

	if ((fd = open(fname, O_RDONLY | O_NONBLOCK)) < 0)
		return (NULL);

	cdinfo = NULL;
	vendor[0] = 0;
	product[0] = 0;
	rev[0] = 0;
	vers = 0;

	if (scsi_inquiry(fd, vendor, product, rev, &vers) < 0)
		return (NULL);
	
	for (i = 0; cdfuncs[i].name; i++)
		if (cdinfo = (*cdfuncs[i].probe)(fd,vendor,product,rev,vers))
			break;

	if (cdinfo == NULL || cdinfo->ntracks == 0) {
		close(fd);
		return(NULL);
	}

	cdinfo->funcs = &cdfuncs[i];

	for (i = 0, tp = cdinfo->tracks; i < cdinfo->ntracks - 1; i++, tp++)
		tp[0].nframes = tp[1].start_frame - tp[0].start_frame;

	cdinfo->tracks[cdinfo->ntracks - 1].nframes = cdinfo->total_frames 
		- cdinfo->tracks[cdinfo->ntracks - 1].start_frame;

	return(cdinfo);
}

/*
 * Close the device and free any memory that was allocated.
 */
int
cdclose(cdinfo)
struct cdinfo *cdinfo;
{
	return((*cdinfo->funcs->close)(cdinfo));
}

/*
 * Play audio from one point to another.  The locations are specified
 * as 75th's of a second from the start of the disk.
 */
int
cdplay(cdinfo, start_frame, end_frame)
struct cdinfo *cdinfo;
int start_frame;
int end_frame;
{
	return((*cdinfo->funcs->play)(cdinfo, start_frame, end_frame));
}

/*
 * Stop playing audio.
 */
int
cdstop(cdinfo)
struct cdinfo *cdinfo;
{
	return((*cdinfo->funcs->stop)(cdinfo));
}

/*
 * Return playing status, including current position.
 */
int
cdstatus(cdinfo, cdstatus)
struct cdinfo *cdinfo;
struct cdstatus *cdstatus;
{
	return((*cdinfo->funcs->status)(cdinfo, cdstatus));
}

/*
 * Eject the caddy, if supported.
 */
int
cdeject(cdinfo)
struct cdinfo *cdinfo;
{
	return((*cdinfo->funcs->eject)(cdinfo));
}

/*
 * Load the caddy, if supported.
 */
int
cdload(cdinfo)
struct cdinfo *cdinfo;
{
	return((*cdinfo->funcs->load)(cdinfo));
}

/*
 * Set the volume level 0 = min, 100 = max
 */
int
cdvolume(cdinfo, volume)
struct cdinfo *cdinfo;
int volume;
{
	return((*cdinfo->funcs->volume)(cdinfo, volume));
}

/*
 * Pause/Resume
 */
int
cdpause(cdinfo, pause_resume)
struct cdinfo *cdinfo;
int pause_resume;
{
	return((*cdinfo->funcs->pause)(cdinfo, pause_resume));
}
