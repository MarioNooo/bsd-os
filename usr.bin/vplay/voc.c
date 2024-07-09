/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI voc.c,v 1.3 1996/11/23 01:03:12 ewv Exp
 */


/*
 * Voc file handling
 */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vplay.h"

/*
 * Check if file is a .voc file and return size of remainder of header,
 * <0 if not a .voc.
 */
int
voc_chkhdr(void *buffer)
{
	VocHeader *vp = buffer;

	if (strstr((const char *)vp->magic, MAGIC_STRING) != NULL) {
		if (vp->version != (0x1233 - vp->coded_ver))
			return (-2);
		return (vp->headerlen - sizeof(VocHeader)); /* 0 mostly */
	}
	return (-1);
}
 
/*
 * Play a .voc file
 */ 
void
play_voc(FILE *fp, char *name)
{
	char btype;
	int override = 0;
	int len = 0;
	int repcnt = 0;
	fpos_t reppos;
	int skip;
	int l;
	int new_dsp_speed;

	if (verbose) 
		fprintf(stderr, "Playing VOC file ...\n");
	if (timelimit != -1)
		fprintf(stderr, "Warning: -t has no effect on .voc files\n");
	if (user.speed != -1) {		/* Speed override */
		fset.speed = user.speed;
		set_speed(user.speed);
	}

	/* Skip the rest of the header (if any) */
	skip = voc_chkhdr(audiobuf);
	while (skip > 0) {	
		l = skip;
		if (l > abuf_size)
			l = abuf_size;
		if (fread(audiobuf, l, 1, fp) != 1) {
			warn("Premature end of file on voc header");
			return;
		}
		skip -= l;
	}

	/* .voc files default to 8 bit mono */
	fset.bits = VOC_SAMPLESIZE;
	set_bits(VOC_SAMPLESIZE);
	fset.stereo = MODE_MONO;
	set_stereo(MODE_MONO);

	for (;;) {
		if ((btype = fgetc(fp)) == EOF) {
			d_printf((stderr, "%s: Premature EOF reading hdr\n",
			   name));
			return;
		}
		switch (btype) {
		case 0:	/* Terminator */
			d_printf((stderr, "Terminator\n"));
			return;

		case 1: { /* Voice data */
			Voc_B1 hdr;
			int len;
			int new_dsp_speed;

			if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
				warn("%s: Premature EOF reading B1 hdr", name);
				return;
			}
			d_printf((stderr, "Voc: B1 block\n"));
			/* Set parms from header */
			if (!override) {
				if (hdr.pack) {
					fprintf(stderr, "%s: Can't play packed "
					    "files\n", name);
					return;
				}
				fset.stereo = MODE_MONO;
				set_stereo(MODE_MONO);
				new_dsp_speed = 1000000 / (256 - hdr.tc);
				if (user.speed == -1) {
					fset.speed = new_dsp_speed;
					set_speed(new_dsp_speed);
				}
				d_printf((stderr, "Speed = %d Hz\n",
				    new_dsp_speed));
			}
			override = 0;
			len = (hdr.len[2] << 16 | hdr.len[1] << 8 |
			    hdr.len[0]) - 2;
			audio_out(fp, len);
			break;
		}

		case 2: /* Continued voice data */
			len = 0;
			if (fread(&len, 3, 1, fp) != 1) {
				warn("%s: Premature EOF reading B2 hdr", name);
				return;
			}
			audio_out(fp, len);
			break;

		case 3: { /* Pause */
			Voc_B3 hdr;
			u_short period;

			if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
				warn("%s: Premature EOF reading B3 hdr", name);
				return;
			}
			period = *(u_short *)hdr.period;
			new_dsp_speed = 1000000 / (256 - hdr.tc);
			fset.speed = new_dsp_speed;
			set_speed(new_dsp_speed);
			if (debug)
				fprintf(stderr, "%d ms silence\n",
				    period * 1000 / dsp.speed);
			write_zeros(period);
			break;
		}
		case 4: { /* Marker */
			Voc_B4 hdr;

			if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
				warn("%s: Premature EOF reading B4 hdr", name);
				return;
			}
			d_printf((stderr, "Marker: %d\n",
			    *(u_short *)hdr.mark));
			break;
		}
		case 6: { /* Start loop */
			Voc_B6 hdr;

			if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
				warn("%s: Premature EOF reading B6 hdr", name);
				return;
			}
			repcnt = *(u_short *)hdr.cnt;
			d_printf((stderr, "Start repeat, count=%d\n", repcnt));
			if (fgetpos(fp, &reppos) == -1) {
				warn("%s: Error getting repeat file pos", name);
				return;
			}
			break;
		}
		case 7: /* Repeat marker */
			if (fread(&len, 3, 1, fp) != 1) {
				warn("%s: Premature EOF reading B7 hdr", name);
				return;
			}
			d_printf((stderr, "Repeat marker: remaining count=%d\n",
			    repcnt));
			if (repcnt == 0)
				break;
			if (repcnt != 0xffff)
				repcnt--;
			if (fsetpos(fp, &reppos) == -1) {
				warn("%s: Error setting file ptr for repeat",
				    name);
				return;
			}
			break;

		case 8: { /* Extended DSP settings */
			Voc_B8 hdr;

			if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
				warn("%s: Premature EOF reading B8 hdr", name);
				return;
			}
			if (hdr.pack != 0) {
				fprintf(stderr, "%s: Can't play packed data\n",
				    name);
				return;
			}
			override = 1;
			fset.stereo = hdr.mode;
			set_stereo(hdr.mode);
			if (user.speed == -1) {
				new_dsp_speed = 256000000L / (65536 - 
				    *(u_short *)hdr.tc);
				if (hdr.mode == MODE_STEREO)
					new_dsp_speed >>= 1;
				fset.speed = new_dsp_speed;
				set_speed(new_dsp_speed);
			}
			break;
		}
		case 9: { /* Sound data - new format */
			Voc_B9 hdr;
			int t;

			if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
				warn("%s: Premature EOF reading B9 hdr", name);
				return;
			}
			t = *(u_short *)hdr.fmt;
			if (t != 0 && t != 0x0004) {
				fprintf(stderr, "%s: Unsupported audio "
				    "format %d\n", name, t);
				return;
			}
			if (t == 0) {
				fset.bits = 8;
				set_bits(8);
			} else {
				fset.bits = 16;
				set_bits(16);
			}
			if (hdr.chan == 1) {
				fset.stereo = MODE_MONO;
				set_stereo(MODE_MONO);
			} else {
				fset.stereo = MODE_STEREO;
				set_stereo(MODE_STEREO);
			}
			if (user.speed == -1) {
				t = *(int *)hdr.rate;
				set_speed(t);
			}
			len = (hdr.len[2] << 16 | hdr.len[1] << 8 |
			    hdr.len[0]) - 12;
			audio_out(fp, len);
			break;
		}
		default:
		case 5: { /* String or unknown */
			int l;
			int resid;

			len = 0;
			if (fread(&len, 3, 1, fp) != 1) {
				warn("%s: Premature EOF reading B5/unk hdr",
				    name);
				return;
			}
			resid = len;
			while (resid) {
				l = resid;
				if (l > abuf_size)
					l = abuf_size;
				if (fread(audiobuf, l, 1, fp) != 1) {
					warn("%s: Premature EOF reading B5/unk "
					    "data", name);
					return;
				}
				resid -= l;
			}
			if (btype == 5) {
				audiobuf[len] = NULL;
				fprintf(stderr,"%s\n", audiobuf);
			} else {
				d_printf((stderr, "Unknown block type=%d "
				    "len=%d\n", btype, len));
			}
			break;
		}
		}
	}
}

/*
 * Write a .VOC-header, this leaves us ready to write DSP data. A drawback
 * is we're limited to 16M files (one voc block).
 *
 * We stick to the old method (B8/B1) so as to be compatible with old apps.
 */
void 
wr_voc_hdr(FILE *fp, u_long cnt)
{
	VocHeader vh;
	Voc_B8 b8;
	Voc_B1 b1;
	u_short s;

	/*
	 * File header
	 */
	strncpy((char *)vh.magic,MAGIC_STRING,20);
	vh.magic[19] = 0x1A;
	vh.headerlen = sizeof(VocHeader);
	vh.version = ACTUAL_VERSION;
	vh.coded_ver = 0x1233 - ACTUAL_VERSION;
	fwrite(&vh, sizeof(vh), 1, fp);

	/*
	 * Write extended block if stereo
	 */
	if (dsp.stereo == MODE_STEREO) {
		fputc(8, fp);	
		bzero(&b8, sizeof(b8));
		b8.len[0] = 4;
		s = (65536 - 256000000L / (dsp.speed << 1));
		*(u_short *)b8.tc = s;
		b8.pack = 0;
		b8.mode = 1;
		fwrite(&b8, sizeof(b8), 1, fp);
	}
	fputc(1, fp);
	cnt += 2;
	*(u_int *)b1.len = cnt;		/* ... this writes a 0 on tc */
	b1.tc = (u_char)(256 - (1000000 / dsp.speed) );
	b1.pack = 0;
	fwrite(&b1, sizeof(b1), 1, fp);
} 

/*
 * Voc termination block
 */
void 
wr_voc_trail(FILE *fp)
{
	fputc(0, fp);
}

/*
 * Write zeroes to audio device to simulate silence
 */
void
write_zeros(unsigned int len)
{
	unsigned int l;
	static char *zbuf = NULL;
 
	if (!zbuf) {
		zbuf = malloc(ZBUF_SIZE);
		bzero(zbuf, ZBUF_SIZE);
	}
	while (len) {
		l = len;
		if (l > ZBUF_SIZE)
			l = ZBUF_SIZE;
		if (write(audio, zbuf, l) != l)
			err(1, "Write to dsp failed");
		len -= l;
	}
} 
