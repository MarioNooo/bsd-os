/*
 * Copyright (c) 1998 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI cput.c,v 1.1 1998/05/01 18:38:14 donn Exp
 */

#include <sys/param.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lflag;

static void
usage()
{

	fprintf(stderr, "usage: cput [-f database] entry [capability [type]]\n");
	exit(2);
}

static void
adddb(char ***dblistp, size_t *dbcountp, char *dbname)
{

	if (*dbcountp == 0)
		if ((*dblistp = malloc(4 * sizeof *dblistp)) == NULL)
			err(2, NULL);
	else if (*dbcountp >= 4 && powerof2(*dbcountp))
		if ((*dblistp =
		    realloc(*dblistp, *dbcountp * 2 * sizeof *dblistp)) == NULL)
			err(2, NULL);
	(*dblistp)[(*dbcountp)++] = dbname;
}

static int
printcap(char *buf, char *capability, int type)
{
	char numbuf[32];
	char *value = NULL;
	char *s;
	long number;

	if (!lflag && type == '=')
		if (cgetstr(buf, capability, &value) < 0)
			return (0);
	if (!lflag && type == '#') {
		if (cgetnum(buf, capability, &number) < 0)
			return (0);
		sprintf(numbuf, "%ld", number);
		value = numbuf;
	}
	if (value == NULL) {
		if ((value = cgetcap(buf, capability, type)) == NULL)
			return (0);
		if ((s = strchr(value, ':')) != NULL)
			*s = '\0';
	}

	if (type == ':')
		puts(capability);
	else
		puts(value);
	return (1);
}

int
main(int argc, char **argv)
{
	char **dblist = NULL;
	size_t dbcount = 0;
	char *buf;
	char *entry;
	char *capability = NULL;
	char *type = "";
	const char typelist[] = "=#:";
	int c;
	size_t i;

	while ((c = getopt(argc, argv, "f:l")) != -1)
		switch (c) {
		case 'f':
			/* XXX permit a path here? */
			adddb(&dblist, &dbcount, optarg);
			break;
		case 'l':
			lflag = 1;
			break;
		default:
			usage();
			break;
		}

	argv += optind;

	if ((buf = getenv("CPUTPATH")) != NULL)
		while ((entry = strsep(&buf, ":")) != NULL)
			adddb(&dblist, &dbcount, entry);

	if (*argv == NULL)
		usage();
	entry = *argv;
	if (*++argv != NULL) {
		capability = *argv;
		if (*++argv != NULL)
			type = *argv;
	}

	if (dbcount == 0)
		errx(2, "%s: no database file specified", entry);

	dblist[dbcount] = NULL;

	switch (cgetent(&buf, dblist, entry)) {
	case 0:
		/* Success */
		break;
	case 1:
		warnx("%s: unresolved `tc=...' expansion", entry);
		break;
	case -1:
		if (capability == NULL)
			return (1);
		errx(2, "%s: not found\n", entry);
		break;
	case -2:
	default:
		err(2, "%s", entry);
		break;
	case -3:
		errx(2, "%s: `tc=...' loop", entry);
		break;
	}

	if (capability == NULL) {
		/* XXX makes unwarranted assumption about buf? */
		puts(buf);
		return (0);
	}

	if (*type != '\0') {
		if (printcap(buf, capability, *type))
			return (0);
		return (1);
	}

	for (i = 0; i < sizeof (typelist) - 1; ++i)
		if (printcap(buf, capability, typelist[i]))
			return (0);
	return (1);
}
