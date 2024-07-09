/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	cpu.c,v 1.11 2001/11/05 23:06:41 polk Exp
 */
#include "cpu.h"

int vflag;
int debug;
char *self;
char *info_file;
char *ovr_file;

#define	NINFO	1		/* Need kern info resident */
#define	NOVR	2		/* Need overrides processed */

static void usage __P((void));

static struct cmtab {
	char *cmd;
	char *args;
	char *desc;
	cfunc_t func;
	int flags;
} cmtab[] = {
	{"cpu",	"",
		"Summarize CPU  bootparam info", disp_cpu,	NINFO },
	{"check",	"",
		"Display CPU information",	cpu_check,	0 },
	{"mp",		"[overrides]",
		"Start all available cpus",	go_mp, 		NINFO | NOVR },
	{"siomode",	"[overrides]",
		"Enter symmetric I/O mode",	siomode, 	NINFO | NOVR },
	{"stat",	"",
		"Display cpu and interrupt status", cpu_stat,	NINFO },
	{"start",	"cpuno",
		"Start a cpu",			cpu_start,	NINFO },
	{"stop",	"cpuno",
		"Stop a cpu",			cpu_stop,	NINFO},
	{"bootparam",	"",
		"Display boot parameters",	bootparam,	NINFO },
	{"intr",	"",
		"Summarize interrupt bootparam info", disp_intr, NINFO },
	{"mpinfo",	"",
		"Summarize MP config info",	disp_mpinfo,	NINFO },
	{"pci",		"",
		"Display PCI device info",	disp_pci,	NINFO },
	{"save",	"filename",
		"Write CPU information to file", save_info,	NINFO },
	{"to",
		"[overrides]", "Test overrides", test_ovr,	0 },
};
#define NCM	(sizeof(cmtab) / sizeof(struct cmtab))


int
main(int ac, char **av)
{
	struct cmtab *cp;
	int c;

	if ((ovr_file = getenv("MPINFO")) == NULL)
		ovr_file = OVR_FILE;
	self = av[0];
	while ((c = getopt(ac, av, "i:vo:Od")) != -1) {
		switch (c) {
		case 'o':
			ovr_file = optarg;
			break;

		case 'O':
			ovr_file = NULL;
			break;

		case 'i':
			/* Load kernel info */
			info_file = optarg;
			break;

		case 'v':
			vflag++;
			break;

		case 'd':
			debug = 1;
			vflag++;
			break;

		default:
			usage();
		}
	}
	ac -= optind;
	av += optind;
	if (ac < 1)
		usage();
	for (cp = cmtab; cp < &cmtab[NCM]; cp++) {
		if (strncmp(av[0], cp->cmd, strlen(av[0])) == 0)
			break;
	}
	if (cp == &cmtab[NCM])
		usage();
	if (cp->flags & NOVR)
		setup_overrides(&av[1], ac - 1);
	if (cp->flags & NINFO) {
		if (info_file)
			load_info(info_file);
		else
			get_kern_info();
	}
	return (cp->func(ac, av));
}

static void
usage(void)
{
	struct cmtab *cp;
	u_char cmd[LINE_MAX];

	fprintf(stderr, "Usage: %s [flags] cmd [args]\n\
	-i infofile	Get machine info from file instead of kernel\n\
	-o file		Get overrides from file (instead of /etc/mp.config)\n\
	-O		Do not use any override info except the command line\n\
	-v		Print verbose information\n\
	-d		Debug mode -- don't issue commands to kerne\n\n\
    Commands:\n", self);
	for (cp = cmtab; cp < &cmtab[NCM]; cp++) {
		snprintf(cmd, sizeof(cmd), "%s %s", cp->cmd, cp->args);
		fprintf(stderr, "\t%-30s %s\n", cmd, cp->desc);
	}
	exit(1);
}
