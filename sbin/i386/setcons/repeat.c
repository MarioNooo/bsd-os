/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI repeat.c,v 1.1 1997/01/02 17:38:38 pjd Exp
 */

#include <sys/types.h>
#include <sys/ioctl.h>

#include <i386/isa/pcconsioctl.h>
#include <i386/isa/ic/i8042.h>

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "extern.h"

/*
 * Definitions and routines for handling keyboard repeat (typematic)
 * parameters.
 */
#define	REPEAT_MASK	0x1f	/* should be in i8042.h */
#define	DELAY_MASK	0x60	/* should be in i8042.h */
#ifndef KTM_3_4_SEC
#define	KTM_3_4_SEC	0x40	/* should be in i8042.h */
#endif

struct repeat {
	float	rate;
	int	code;
};
static struct repeat repeat_code[] = {
	{ 2.0,	KTM_2_0_RPS },
	{ 2.1,	KTM_2_1_RPS },
	{ 2.3,	KTM_2_3_RPS },
	{ 2.5,	KTM_2_5_RPS },
	{ 2.7,	KTM_2_7_RPS },
	{ 3.0,	KTM_3_0_RPS },
	{ 3.3,	KTM_3_3_RPS },
	{ 3.7,	KTM_3_7_RPS },
	{ 4.0,	KTM_4_0_RPS },
	{ 4.3,	KTM_4_3_RPS },
	{ 4.6,	KTM_4_6_RPS },
	{ 5.0,	KTM_5_0_RPS },
	{ 5.5,	KTM_5_5_RPS },
	{ 6.0,	KTM_6_0_RPS },
	{ 6.7,	KTM_6_7_RPS },
	{ 7.5,	KTM_7_5_RPS },
	{ 8.0,	KTM_8_0_RPS },
	{ 8.6,	KTM_8_6_RPS },
	{ 9.2,	KTM_9_2_RPS },
	{ 10.0,	KTM_10_0_RPS },
	{ 10.9,	KTM_10_9_RPS },
	{ 12.0,	KTM_12_0_RPS },
	{ 13.3,	KTM_13_3_RPS },
	{ 15.0,	KTM_15_0_RPS },
	{ 16.0,	KTM_16_0_RPS },
	{ 17.1,	KTM_17_1_RPS },
	{ 18.5,	KTM_18_5_RPS },
	{ 20.0,	KTM_20_0_RPS },
	{ 21.8,	KTM_21_8_RPS },
	{ 24.0,	KTM_24_0_RPS },
	{ 26.7,	KTM_26_7_RPS },
	{ 30.0,	KTM_30_0_RPS },
	{ 0.0,	0 },
};
static struct repeat delay_code[] = {
	{ 0.25, KTM_1_4_SEC },
	{ 0.50, KTM_1_2_SEC },
	{ 0.75, KTM_3_4_SEC },
	{ 1.00, KTM_1_SEC },
	{ 0.0, 0 }
};

static int	get_repeat __P((int));
static int	get_repeat_code __P((char *, struct repeat *));
static void	repeat_usage __P((void));
static int	set_repeat __P((int, int));
static void	show_repeat __P((int));

int
repeat(argc, argv, fd)
	int argc, fd;
	char *argv[];
{
	int code;

	code = get_repeat(fd);
	if (argc == 0) {
		show_repeat(code);
		return (0);
	}
	for (; argc >= 2; argc -= 2, argv += 2) {
		if (strcmp(argv[0], "rate") == 0) {
			code = (code & DELAY_MASK) |
			    get_repeat_code(argv[1], repeat_code);
			continue;
		}
		if (strcmp(argv[0], "delay") == 0) {
			code = (code & REPEAT_MASK) |
			    get_repeat_code(argv[1], delay_code);
			continue;
		}
		goto usage;
	}
	if (argc) {
usage:		repeat_usage();
		return (1);
	}

	return (set_repeat(fd, code));

}

static int
get_repeat_code(name, table)
	char *name;
	struct repeat *table;
{
	float rate;

	for (rate = atof(name); table->rate != 0.0; table++)
		if (table->rate >= rate)
			return (table->code);
	return (table[-1].code);
}

static void
show_repeat(code)
	int code;
{
	struct repeat *table;

	printf("repeat rate ");
	for (table = repeat_code; table->rate != 0.0; table++)
		if (table->code == (code & REPEAT_MASK)) {
			printf("%.1f", table->rate);
			break;
		}
	if (table->rate == 0.0)
		printf("???(%d)", code & REPEAT_MASK);

	printf(" delay ");
	for (table = delay_code; table->rate != 0.0; table++)
		if (table->code == (code & DELAY_MASK)) {
			printf("%.2f\n", table->rate);
			break;
		}
	if (table->rate == 0.0)
		printf("???(%d)\n", code & DELAY_MASK);
}

static int
get_repeat(fd)
	int fd;
{
	int code;

	if (ioctl(fd, PCCONIOCGRPT, &code) != 0)
		err(1, "get repeat");
	return (code);
}

static int
set_repeat(fd, code)
	int fd, code;
{

	if (ioctl(fd, PCCONIOCSRPT, &code) != 0)
		err(1, "set repeat");
	return (0);
}

void
repeat_help()
{
	struct repeat *table;
	int i;

	printf("repeat category:\n");
	printf("\t    rate -- characters per second\n");
	printf("\t   delay -- seconds\n");
	printf(
"\n\tInput values are rounded up as needed, or down to the highest value.\n\n");
	printf("\tValues for rate (characters per second):\n");

#define	COLUMNS	6
	for (i = 0, table = repeat_code; table->rate != 0.0; table++) {
		printf("\t%.1f", table->rate);
		if (++i % COLUMNS == 0)
			printf("\n");
	}
	if (i % COLUMNS != 0)
		printf("\n");

	printf("\n\tValues for delay (seconds):\n");
	for (i = 0, table = delay_code; table->rate != 0.0; table++) {
		printf("\t%.2f", table->rate);
		if (++i % 4 == 0)
			printf("\n");
	}
	printf("\n");
}

static void
repeat_usage()
{
	fprintf(stderr,
	    "usage: setcons repeat [rate chars-per-sec] [delay sec]\n");
}
