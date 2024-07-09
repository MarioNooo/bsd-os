/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1995 Berkeley Software Design, Inc. 
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * Audio mixer controller program
 *
 * BSDI mixer.c,v 1.6 2001/10/03 17:29:57 polk Exp
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/soundcard.h>
#include <sys/fcntl.h>

static void print_all __P((void));
static int keymatch __P((char *));
static void pdevs __P((int, char *));
static void pval __P((int));
static void precsrc __P((void));
static void usage __P((char *self));

#define RDEV	256

static char *device_names[] = SOUND_DEVICE_NAMES;

static char *device_descs[] = {
	"Master volume",
	"Bass",
	"Treble",
	"On-board Sythesizer",
	"Primary DAC",
	"PC speaker",
	"Main line in",
	"Microphone",
	"CD Player",
	"Recording monitor",
	"Secondary DAC",
	"Record level",
	"Input gain",
	"Output gain",
	"Line input 1",
	"Line input 2",
	"Line input 3",
	"Digital input 1",
	"Digital input 2",
	"Digital input 3",
	"Phone input",
	"Phone output",
	"TV/Video input",
	"Radio input",
	"Monitor volume",
	"3D depth/space",
	"3D center",
	"MIDI"
};

int devmask;
int recmask;
int stereomask;
int fd = -1;
char	mixname[32] = "/dev/mixer";
int vflag;

int
main(int ac, char **av)
{
	int c;
	int mixer = 0;
	int idx;
	int idxr;
	int val;
	int left, right;
	char *term = "";
	char *sep = "";
	char *p;
	char *ap;

	while ((c = getopt(ac, av, "m:v")) != EOF) {
		switch (c) {
		case 'm':
			mixer = atoi(optarg);
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			usage(av[0]);
		}
	}
	ac -= optind;
	av += optind;

	if (mixer != 0)
		sprintf(mixname, "/dev/mixer%d", mixer);

	if ((fd = open(mixname, O_RDWR)) < 0)
		err(1, "Can't open %s", mixname);

	/* Get device and record mask */
	if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devmask) < 0)
		err(1, "SOUND_MIXER_READ_DEVMASK failed");
	if (ioctl(fd, SOUND_MIXER_READ_RECMASK, &recmask) < 0)
		err(1, "SOUND_MIXER_READ_RECMASK failed");
	if (ioctl(fd, SOUND_MIXER_READ_STEREODEVS, &stereomask) < 0)
		err(1, "SOUND_MIXER_READ_STEREODEVS failed");

	if (ac == 0) {
		print_all();
		exit(0);
	}

	while (ac) {
		idx = keymatch(av[0]);
		if (idx < 0) {
			fprintf(stderr, "Invalid keyword: %s\n", av[0]);
			exit(1);
		}
		if (!(devmask & 1 << idx)) {
			fprintf(stderr, "Unsupported device: %s\n", av[0]);
			exit(1);
		}

		/*
		 * Sort of hokey: if there are no arguments left or the
		 * next argument isn't a number and the current argument
		 * isn't the record device keyword then print info on the
		 * channel.
		 */
		if (ac == 1 || (!strchr("0123456789", *av[1]) && 
		    (idx != RDEV))) {
			fputs(sep, stdout);
			pval(idx);
			term = "\n";
			sep = " ";
			ac--;
			av++;
			continue;
		}

		/* Set a channel */
		if (idx == RDEV) {
			/* Recording devices */
			val = 0;
			p = av[1];

			while ((ap = strsep(&p, ",")) != NULL) {
				idxr = keymatch(ap);
				if (idxr < 0 || idxr == RDEV) {
					fprintf(stderr, "Unknown recording "
					    "source: %s\n", ap);
					exit(1);
				}
				left = 1 << idxr;
				if ((recmask & left) == 0) {
					fprintf(stderr, "Not able to record "
					    "from %s\n", device_names[idxr]);
					exit(1);
				}
				val |= left;
			}
			if (ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &val) < 0)
				err(1, "SOUND_MIXER_WRITE_RECSRC failed");
		} else {
			/* Numeric value channels */
			if (strchr(av[1], ':'))
				sscanf(av[1], "%d:%d", &left, &right);
			else {
				sscanf(av[1], "%d", &left);
				right = left;
			}
			if (left < 0 || left > 100 ||
			    right < 0 || right > 100) {
				fprintf(stderr, "Value out of range "
				    "(0 ... 100)\n");
				exit(1);
			}
			val = left | right << 8;
			if (ioctl(fd, MIXER_WRITE(idx), &val) < 0)
				err(1, "MIXER_WRITE(%d) failed", idx);
		}
		ac -= 2;
		av += 2;
	}
	fputs(term, stdout);
	if (vflag)
		print_all();
	exit(0);
}

/*
 * Print all settings and available devices
 */
void
print_all()
{
	int idx;
	int mask;
	int val;
	int left, right;

	printf("%-10s %-30s %s\n", "Device", "Description", "Value");
	idx = 0;
	mask = devmask;
	for (idx = 0; idx < SOUND_MIXER_NRDEVICES; idx++) {
		mask = 1 << idx;
		if (devmask & mask) {
			if (ioctl(fd, MIXER_READ(idx), &val) < 0)
				err(1, "MIXER_READ(%d) failed", idx);
			left = val & 0xff;
			right = val >> 8 & 0xff;
			printf("%-10s %-30s ", device_names[idx], device_descs[idx]);
			if ((stereomask & mask) == 0)
				printf("%d\n", left);
			else
				printf("%d:%d\n", left, right);
		}
	}
	printf("%-10s %-30s ", "rdev", "Recording source");
	precsrc();
	printf("\n\nValid recording devices: ");
	pdevs(recmask, ", ");
	printf("\n");
}

/*
 * Find a device number based on a (possibly partial) name.
 */
int
keymatch(char *key)
{
	int hit;
	int idx;
	int hit_idx;

	if (strncmp(key, "rdev", strlen(key)) == 0) {
		hit = 1;
		hit_idx = RDEV;
	} else {
		hit = 0;
		hit_idx = -1;
	}
	for (idx = 0; idx < SOUND_MIXER_NRDEVICES; idx++) {
		if (strncmp(key, device_names[idx], strlen(key)) == 0) {
			if (strlen(device_names[idx]) == strlen(key))
				return (idx);
			hit++;
			hit_idx = idx;
		}
	}
	if (hit > 1) {
		fprintf(stderr, "\"%s\" is not unique\n", key);
		return (-1);
	}
	return (hit_idx);
}

/*
 * Print all devices specified in the passed bit vector
 */
void
pdevs(int mask, char *sep)
{
	int idx = 0;
	char *tsep = "";

	while (mask != 0) {
		if (mask & 0x1) {
			printf("%s%s", tsep, device_names[idx]);
			tsep = sep;
		}
		mask >>= 1;
		idx++;
	}
}

/*
 * Print the current value of a channel
 */
void
pval(int idx)
{
	int val;
	int left, right;

	if (idx == RDEV) {
		printf("%s ", device_names[idx]);
		precsrc();
		return;
	}
	if (ioctl(fd, MIXER_READ(idx), &val) < 0)
		err(1, "MIXER_READ(%d) failed", device_descs[idx]);
	left = val & 0xff;
	right = val >> 8 & 0xff;
	if (stereomask & 1 << idx)
		printf("%s %d:%d", device_names[idx], left, right);
	else
		printf("%s %d", device_names[idx], left);
}

/*
 * Print current recording sources
 */
void
precsrc()
{
	int recsrc;

	if (ioctl(fd, SOUND_MIXER_READ_RECSRC, &recsrc) < 0)
		err(1, "SOUND_MIXER_READ_RECSRC failed");
	pdevs(recsrc, ",");
}

void
usage(char *self)
{
	fprintf(stderr,"Usage: %s [-v] [-m mixer-num] [device [val]] [...]\n",
	    self);
	exit(1);
}

