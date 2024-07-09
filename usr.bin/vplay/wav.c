/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI wav.c,v 1.6 1997/10/01 05:53:29 ewv Exp
 */

/*
 * Routines to handle wav files
 */
#include <stdio.h>

#include "vplay.h"

/*
 * Check if a wav file, if so set speed, stereo, etc. Return 0 if .wav
 * or -1 if not.
 */
int
wav_chkhdr(void *buffer, char *name)
{
	WaveHeader *wp = buffer;

	if (wp->main_chunk != RIFF || wp->chunk_type != WAVE ||
	    wp->sub_chunk != FMT) 
		return (-1);

	if (wp->format != PCM_CODE) {
		fprintf(stderr, "%s: not PCM-coded, can't play\n", name);
		return (-1);
	}

	if (wp->modus > 2) {
		fprintf(stderr, "%s: too many tracks (%d), 2 max\n", name,
		    wp->modus);
		return (-1);
	}
	return (0);
}

/*
 * Find start of data and length
 */
u_long
wav_finddata(FILE *fp, char *name)
{
	u_long	chdr[2];
	u_long	len;
	char	bufr[1024];
	u_long	l;

	for (;;) {
		if (fread(chdr, sizeof(chdr), 1, fp) != 1)
			return (0);
		if (chdr[0] == DATA)
			return (chdr[1]);
		len = chdr[1];
		if (len > 16384) {
			fprintf(stderr, "%s: Wav file corrupt, bogus chunk len: "
			    "chunk=0x%lx len=0x%lx\n", name, chdr[0], chdr[1]);
			return(0);
		}
		while (len) {
			l = len;
			if (l > 1024)
				l = 1024;
			if (fread(bufr, l, 1, fp) != 1) {
				fprintf(stderr, "Wav file corrupt: EOF "
				    "skipping chunk=0x%lx len=0x%lx\n", 
				    chdr[0], chdr[1]);
				return (0);
			}
			len -= l;
		}
	}
}
 
/*
 * write a WAVE-header
 */
void
wr_wav_hdr(FILE *fp, u_long cnt)
{
	WaveHeader wh;
	u_long dhdr[2];

	wh.main_chunk = RIFF;
	wh.length     = cnt + sizeof(WaveHeader) + sizeof(dhdr) - 8; 
	wh.chunk_type = WAVE;
	wh.sub_chunk  = FMT;
	wh.sc_len     = 16;
	wh.format     = PCM_CODE;
	wh.modus      = dsp.stereo ? 2 : 1;
	wh.sample_fq  = dsp.speed;
	wh.byte_p_spl = (dsp.bits == 8) ? 2 : 4;
	wh.byte_p_sec = dsp.speed * wh.modus * wh.byte_p_spl;
	wh.bit_p_spl  = dsp.bits;
	fwrite(&wh, sizeof(wh), 1, fp);

	dhdr[0] = DATA;
	dhdr[1] = cnt;
	fwrite(dhdr, sizeof(dhdr), 1, fp);
}

/*
 * Set dsp parameters from wav header
 */
void
set_wavparms(void *buf)
{
	WaveHeader *wh = buf;
	fset.speed = wh->sample_fq;
	fset.bits = wh->bit_p_spl;
	fset.stereo = (wh->modus == 2) ? MODE_STEREO : MODE_MONO;
}
