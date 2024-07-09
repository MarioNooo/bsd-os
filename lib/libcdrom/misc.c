/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI misc.c,v 2.4 1996/06/28 23:36:26 cp Exp
 */

/*
 * This file contains various helper functions for libcdrom.  It could
 * have gone in common.c, but putting these here avoids error messages
 * about cycles in tsort.
 */

#include <cdrom.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <dev/scsi/scsi.h>
#include <dev/scsi/scsi_ioctl.h>

#include "devs.h"

/*
 * Convert between bcd values as used in the cdrom table of contents and
 * regular ints.
 */
int
bcd_to_int(val)
	int val;
{
	return (((val >> 4) & 0xf) * 10 + (val & 0xf));
}

int
int_to_bcd(val)
	int val;
{
	return (((val / 10) << 4) | (val % 10));
}

/*
 * Convert between frame (which is essentially a logical block number
 * from the start of the disk), and a (minute, second, frame) tuple.
 * The confusing terminology is inherited from the cd specs.
 */
int
msf_to_frame(minute, second, frame)
	int minute, second, frame;
{
	return (minute * FRAMES_PER_MINUTE + second * FRAMES_PER_SECOND +
	    frame);
}

void
frame_to_msf(frame, minp, secp, framep)
	int frame;
	int *minp;
	int *secp;
	int *framep;
{
	*minp = frame / FRAMES_PER_MINUTE;
	*secp = (frame % FRAMES_PER_MINUTE) / FRAMES_PER_SECOND;
	*framep = frame % FRAMES_PER_SECOND;
}

struct cdinfo *
make_cdinfo(fd, ntracks)
	int fd;
	int ntracks;
{
	struct cdinfo *cdinfo;

	if ((cdinfo = (struct cdinfo *)calloc(1, sizeof *cdinfo)) == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	cdinfo->fd = fd;
	cdinfo->ntracks = ntracks;
	cdinfo->tracks = (struct track_info *)
		calloc(ntracks, sizeof (struct track_info));

	if (cdinfo->tracks == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	return (cdinfo);
}

static void
scsi_strcpy (dest, src, len)
	char *dest;
	char *src;
	int len;
{
	while (len > 0 && src[len-1] == ' ')
		len--;

	strncpy (dest, src, len);
	dest[len] = 0;
}

int
scsi_inquiry(fd, vendor, ident, rev, vers)
	int fd;
	char *vendor;
	char *ident;
	char *rev;
	int *vers;
{
	u_char buf[36];
	u_char cdb[6];

	bzero (cdb, sizeof cdb);
	cdb[0] = 0x12;
	cdb[4] = sizeof buf;

	if (scsi_cmd (fd, cdb, 6, buf, sizeof buf) < 0)
		return (-1);

	scsi_strcpy (vendor, buf + 8, 8);
	scsi_strcpy (ident, buf + 16, 16);
	scsi_strcpy (rev, buf + 32, 4);
	*vers = buf[2] & 7;
	return (0);
}

int
scsi_cmd(fd, cdb, cdblen, data, datalen)
	int fd;
	u_char *cdb, *data;
	int cdblen, datalen;
{
	char ch;
	int n;
	scsi_user_cdb_t suc;

	bzero (&suc, sizeof suc);
	suc.suc_flags = SUC_READ;
	suc.suc_cdblen = cdblen;
	bcopy(cdb, suc.suc_cdb, cdblen);
	suc.suc_datalen = datalen;
	suc.suc_data = (caddr_t) data;

		
	if (ioctl(fd, SCSIRAWCDB, &suc) < 0)
		return (-1);

	if (suc.suc_sus.sus_status != 0)
		return (-1);

	return (0);
}

int
scsi_cmd_write(fd, cdb, cdblen, data, datalen)
	int fd;
	u_char *cdb, *data;
	int cdblen, datalen;
{
	char ch;
	int n;
	scsi_user_cdb_t suc;

	bzero (&suc, sizeof suc);
	suc.suc_flags = SUC_WRITE;
	suc.suc_cdblen = cdblen;
	bcopy(cdb, suc.suc_cdb, cdblen);
	suc.suc_datalen = datalen;
	suc.suc_data = (caddr_t)data;

	if (ioctl(fd, SCSIRAWCDB, &suc) < 0)
		return (-1);

	if (suc.suc_sus.sus_status != 0)
		return (-1);

	return (0);
}

int
scsi_cmd_read(fd, cdb, cdblen, data, datalen)
	int fd;
	u_char *cdb, *data;
	int cdblen, datalen;
{
	char ch;
	int n;
	scsi_user_cdb_t suc;

	bzero (&suc, sizeof suc);
	suc.suc_flags = SUC_READ;
	suc.suc_cdblen = cdblen;
	bcopy(cdb, suc.suc_cdb, cdblen);
	suc.suc_datalen = datalen;
	suc.suc_data = (caddr_t)data;

	if (ioctl(fd, SCSIRAWCDB, &suc) < 0)
		return (-1);

	if (suc.suc_sus.sus_status != 0)
		return (-1);

	return (datalen - suc.suc_datalen);
}
