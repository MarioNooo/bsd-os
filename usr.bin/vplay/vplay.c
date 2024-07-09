/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI vplay.c,v 1.7 1997/10/01 20:02:23 ewv Exp
 */

/*
 * play and record sounds files. Formats supported:
 *	.voc	Creative voice file
 *	.wav	Windoze wave
 *	raw
 *
 * Based on the work of: Michael Beck - beck@informatik.hu-berlin.de
 */
#define USAGE "Usage: %s [flags] file ...\n\
	-f file		Audio device (def: /dev/dsp)\n\
	-T rec_fmt	Recording format (def: based on filename)\n\
				raw, voc, or wav\n\
	-s speed	Dsp speed (Hz) (def: 8Khz or from file hdr)\n\
	-b bits		Sample size (8 or 16) (def: 8)\n\
	-S		Stereo
	-M		Mono (default for recording)
	-t secs		Play/record time limit per file (def: unlim)\n\
	-w secs		Time to wait for audio device not busy (def: 0)\n\
	-v		Verbose status reports\n\
	-d		Debug ouput\n"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <err.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <signal.h>

#include "vplay.h"

int verbose = 0;
int debug = 0;

char *adev = DEFAULT_DEV;
int audio;

int abuf_size;
u_char *audiobuf;

char *command;

/* Current dsp settings */
DspSettings dsp = { -1, -1, -1 };

/* Setting from the file being read */
DspSettings fset = { -1, -1, -1 };

/* User requested dsp settings */
DspSettings user = { -1, -1, -1 };
int timelimit = -1;
AudioType sound_type = Unk;


void catch_sigint() {
	ioctl(audio, SNDCTL_DSP_RESET, 0);
	exit (0);
}

int 
main(int argc, char **argv)
{
	char c;
	int waitsec = 0;
	int omode;
	Direction dir;

	command = argv[0];
	if (strstr (argv[0], "vrec")) {
		dir = Record;
		omode = O_RDONLY;
	} else if (strstr (argv[0], "vplay")) {
		dir = Play;
		omode = O_WRONLY;
	} else {
		fprintf(stderr, "Error: command should be named either "
		    "vrec or vplay\n");
		exit(1);
	}

	while ((c = getopt (argc, argv, "f:T:s:b:SMt:w:vd")) != EOF) {
		switch (c) {
		/* Audio device */
		case 'f':
			adev = optarg;
			break;

		/*
		 * File format (overrides filename extension for record)
		 */
		case 'T':
			switch (optarg[0]) {
			case 'r':
				sound_type = Raw;
				break;
			case 'v' :
				sound_type = Voc;
				break;
			case 'w':
				sound_type = Wav;
				break;
			default:
				fprintf(stderr, "%s: Invalid sound type: %s\n",
				    command, optarg);
				exit(1);
			}
			break;

		/*
		 * Audio parameter overrides
		 */
		case 's':
			user.speed = atoi(optarg);
			if (user.speed < 300)
				user.speed *= 1000;
			break;
		case 'b':
			user.bits = atoi(optarg);
			if (user.bits != 8 && user.bits != 16) {
				fprintf(stderr, "%s: Invalid bitwidth %d\n",
				    command, user.bits);
				exit(1);
			}
			break;
		case 'S':
			user.stereo = MODE_STEREO;
			break;
		case 'M':
			user.stereo = MODE_MONO;
			break;

		case 't':
			timelimit = strtol(optarg, NULL, 0);
			break;

		case 'w':
			waitsec = atoi(optarg);
			if (waitsec < 1)
				waitsec = 1;
			break;

		case 'v':
			verbose = 1;
			break;
		case 'd':
			debug = 1;
			verbose = 1;
			break;
		default:
			usage();
			break;
		}
	}

	/*
	 * Grab the audio device, retry if the unit is busy
	 */
	waitsec *= 2;
	while ((audio = open(adev, omode, 0)) < 0) {
		struct timeval tv = { 0, 500000 };

		if (errno != EBUSY || waitsec == 0)
			err(1, "Can't open %s", adev);
		select(1, NULL, NULL, NULL, &tv);
		waitsec--;
	}

	if (ioctl(audio, SNDCTL_DSP_GETBLKSIZE, &abuf_size) < 0)
		err(1, "SNDCTL_DSP_GETBLKSIZE");

	if (abuf_size < 4096 || abuf_size > 65536) {
		fprintf(stderr, "Using default buffer size (4096)\n"); 
		abuf_size = 4096;
	}
 
	audiobuf = (u_char *)malloc(abuf_size);
 
	signal (SIGINT, catch_sigint);
	if (optind > argc - 1) {
		if (dir == Play)
			play_file(NULL);
		else
			record_file(NULL);
	} else {
		while (optind <= argc - 1) {
			if (dir == Play) 
				play_file(argv[optind++]);
			else
				record_file(argv[optind++]);
		}
	}
	close(audio);
	return (0);
}

/*
 * Play a sound file
 */
void 
play_file(char *name)
{
	FILE *fp;
	unsigned long count;
	int new_count;

	if (!name) {
		fp = stdin;
		name = "stdin";
	} else if ((fp = fopen (name, "r")) == NULL) {
		warn("Can't open %s", name);
		return;
	}
	setvbuf(fp, NULL, _IOFBF, 2 * abuf_size);

	/*
	 * Ignore header if raw is specified
	 */
	if (sound_type == Raw) {
		set_bits(user.bits);
		set_stereo(user.stereo);
		set_speed(user.speed);
		count = calc_count();
		fset = user;		/* File input is what user requested */
		if (verbose)
			fprintf(stderr, "Playing raw data: %d Hz %d bits %s\n",
			    dsp.speed, dsp.bits, 
			    dsp.stereo == MODE_STEREO ? "Stereo" : "Mono");
		audio_out(fp, count);
		goto out;
	}

	/*
	 * For playback we ignore the sound_type and depend on the header
	 * if its not specified as raw.
	 */
	if (fread(audiobuf, sizeof(VocHeader), 1, fp) != 1)
		goto fail;

	/* check for .voc file */
	if (voc_chkhdr(audiobuf) >= 0) {
		play_voc(fp, name);
		goto out;
	}

	/* Check for .wav file (need more header) */
	if (fread(&audiobuf[sizeof(VocHeader)], sizeof(WaveHeader) -
	    sizeof(VocHeader), 1, fp) != 1)
		goto fail;
	if (wav_chkhdr(audiobuf, name) >= 0) {
		WaveHeader *wp = (WaveHeader *)audiobuf;
		if (wp->sc_len > 16)
			fseek(fp, wp->sc_len-16, 1);
		/* Get audio params from header */
		set_wavparms(audiobuf);	

		/*
		 * Set up DSP and apply user overrides.
		 *
		 * Speed is a special case override, the DSP just gets set up
		 * for the requested speed regardless of the file's speed
		 * (for bits and stereo audio_out converts on the fly).
		 */

		set_bits(fset.bits);
		if (user.bits != -1)
			set_bits(user.bits);
		set_stereo(fset.stereo);
		if (user.stereo != -1)
			set_stereo(user.stereo);
		set_speed(fset.speed);
		if (user.speed != -1)
			set_speed(user.speed);

		/* Find start of data (skip extraneous headers) */
		if ((count = wav_finddata(fp, name)) == 0)
			goto out;

		if (timelimit > 0) {
			new_count = calc_count();
			if (new_count < count)
				count = new_count;
		}
		if (verbose)
			fprintf(stderr, "Playing Wav file: %d Hz %d bits %s\n",
			    dsp.speed, dsp.bits, 
			    dsp.stereo == MODE_STEREO ? "Stereo" : "Mono");
		audio_out(fp, count);
		goto out;
	}
fail:
	fprintf(stderr, "%s: File format unrecognized\n", name);
out:
	if (fp != stdin)
		fclose(fp);
	return;
}

/*
 * Record a file
 *
 * Determine type first from command line flags, if not that then check
 * the file extension. If all fails record with raw defaults.
 */
void
record_file(char *name)
{
	FILE *fp;
	u_long count;
	char *p;

	if (name == NULL) {
		fp = stdout;
		name = "stdout";
	} else if ((fp = fopen(name, "w")) == NULL) {
		warn("Can't create %s", name);
		return;
	}
	setvbuf(fp, NULL, _IOFBF, abuf_size);

	/* Use filename suffix if we still don't know the type */
	if (sound_type == Unk) {
		p = strstr(name, ".wav");
		if (p != NULL && p[4] == NULL)
			sound_type = Wav;
	}
	if (sound_type == Unk) {
		p = strstr(name, ".voc");
		if (p != NULL && p[4] == NULL)
			sound_type = Voc;
	}
	if (sound_type == Unk)
		sound_type = Raw;

	set_bits(user.bits);
	set_stereo(user.stereo);
	set_speed(user.speed);
	count = calc_count() & ~1;		/* Make even */

	/* Write the header */
	switch (sound_type) {
	case Wav:
		wr_wav_hdr(fp, count);
		break;

	case Voc:
		if (dsp.bits == 16) {
			fprintf(stderr, "%s: Recording in 8 bit mode (due to "
			    "voc file format)\n", name);
			set_bits(8);
			count = calc_count() & ~1;
		}
		if (count > VOC_MAXSEG) {
			if (count != -2)
				fprintf(stderr, "%s: Voc recording truncated "
				    "to 16Mb\n", name);
			count = VOC_MAXSEG;
		}
		wr_voc_hdr(fp, count);
		break;
	default:
		break;
	}

	/* Record the data */
	audio_in(fp, count);

	/* Write any trailing data */
	switch (sound_type) {
	case Voc:
		wr_voc_trail(fp);
		break;
		
	default:
		break;
	}
	if (fp != stdout)
		fclose(fp);
} 

/*
 * Output audio data... apply transformations if needed
 */
void
audio_out(FILE *fp, u_long count)
{
	unsigned long this_len;

	while (count != 0) {
		this_len = count;
		if (this_len > abuf_size)
			this_len = abuf_size;
		if (fread(audiobuf, this_len, 1, fp) != 1)
			return;
		count -= this_len;

		/* Convert buffer to mono if needed */
		if (dsp.stereo == MODE_MONO && fset.stereo == MODE_STEREO)
			this_len = cvt_to_mono(audiobuf, this_len, fset.bits);

		/* Convert 16->8 if needed */
		if (dsp.bits == 8 && fset.bits == 16)
			this_len = cvt_to_8(audiobuf, this_len);

		if (write(audio, audiobuf, this_len) != this_len)
			err(1, "Write to audio device failed");
	}
}

/*
 * Input audio data... apply transformations if needed
 */
void
audio_in(FILE *fp, u_long count)
{
	u_long this_len;

	while (count != 0) {
		this_len = count;
		if (this_len > abuf_size)
			this_len = abuf_size;
		if (read(audio, audiobuf, this_len) != this_len) {
			warn("Error reading from DSP");
			return;
		}
		if (fwrite(audiobuf, this_len, 1, fp) != 1)
			err(1, "Write to audio file failed");
		count -= this_len;
	}
}

/*
 * Set stereo mode (or fake it on output)
 */
void
set_stereo(int new)
{
	if (new == -1)
		new = MODE_MONO;
	if (new == dsp.stereo)
		return;

	dsp.stereo = new;
	sync_dsp();
	if (verbose)
		fprintf(stderr, "Attempting to set stereo mode to %d\n", new);
	if (ioctl(audio, SNDCTL_DSP_STEREO, &dsp.stereo) < 0)
		err(1, "Error setting stereo/mono mode to %d", new);
	if (new == dsp.stereo)
		return;
	if (new == MODE_STEREO && dsp.stereo == MODE_MONO) {
		if (verbose)
			fprintf(stderr, "Converting stereo to mono\n");
		return;
	}
	err(1, "SNDCTL_DSP_STEREO failed");
}

/*
 * Set number of bits per sample (or fake on output)
 */
void
set_bits(int new)
{
	if (new == -1)
		new = 8;
	if (new == dsp.bits) 
		return;
	dsp.bits = new;
	sync_dsp();
	if (verbose)
		fprintf(stderr, "Attempting to set %d bits\n", new);
	if (ioctl(audio, SNDCTL_DSP_SAMPLESIZE, &dsp.bits) < 0)
		err(1, "Error setting number of bits per sample to %d", new);
	if (dsp.bits == new)
		return;
	if (new == 16 && dsp.bits == 8) {
		if (verbose)
			fprintf(stderr, "Converting 16 bit to 8 bit\n");
		return;
	}
	err(1, "Failed to set sample size to %d bits, got %d\n", new, dsp.bits);
}

/*
 * Set DSP speed
 */
void
set_speed(int new)
{
	if (new == -1)
		new = DEFAULT_DSP_SPEED;
	if (new == dsp.speed)
		return;
	sync_dsp();
	dsp.speed = new;
	if (verbose)
		fprintf(stderr, "Attempting to set speed %d Hz\n", new);
	if (ioctl(audio, SNDCTL_DSP_SPEED, &dsp.speed) < 0)
		err(1, "%s: unable to set speed to %d", command, new);
	if (dsp.speed != new) {
		if (verbose)
			fprintf(stderr, "Speed %d not available, %d offered\n",
			    new, dsp.speed);
		/* Accept the available speed if its within 5% */
		if (abs((dsp.speed * 100 / new) - 100) > 5) {
			fprintf(stderr, "%s: sample rate %d not available, they want us to use %d\n",
			    command, new, dsp.speed);
			exit(1);
		}
	}
}

/*
 * Synchronize the dma buffers (wait for them to drain)
 */
void 
sync_dsp(void)
{
	if (ioctl(audio, SNDCTL_DSP_SYNC, NULL) < 0)
		err(1, "SNDCTL_DSP_SYNC failed");
}

/*
 * Convert from time limit to number of bytes (using current dsp settings)
 */
u_long
calc_count()
{
	u_long count;

	if (timelimit <= 0)
		count = -1;
	else {
		count = timelimit * dsp.speed;
		if (dsp.stereo)
			count <<= 1;
		if (dsp.bits == 16)
			count <<= 1;
	}
	return (count);
}

/*
 * Convert a PCM audio buffer from stero to mono
 */
int
cvt_to_mono(u_char *buf, int count, int bits)
{
	int ret = count >> 1;

	if (bits == 8) {
		u_char *src = buf;
		while (count > 0) {
			*buf++ = ((u_int)src[0] + (u_int)src[1]) >> 1;
			src += 2;
			count -= 2;
		}
	} else {
		short *src = (short *)buf;
		short *dst = (short *)buf;
		count >>= 1;
		while (count > 0) {
			*dst++ = ((int)src[0] + (int)src[1]) >> 1;
			src += 2;
			count -= 2;
		}
	}
	return (ret);
}

/*
 * Convert from 16 bit signed to 8 bit unsigned
 */
int
cvt_to_8(void *buf, int count)
{
	u_char *src = buf;
	u_char *dst = buf;
	int ret = count >> 1;

	count >>= 1;
	src++;			/* Do all high order bytes */
	while (count > 0) {
		*dst++ = *src + 128;
		src += 2;
		count--;
	}
	return (ret);
}

/*
 * Usage message
 */
void
usage()
{
	fprintf(stderr, USAGE, command);
	exit(1);
}
