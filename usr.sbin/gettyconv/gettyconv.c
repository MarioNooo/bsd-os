/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gettyconv.c,v 1.1 1996/09/30 17:46:33 prb Exp
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char *gettytab[] = { "/etc/gettytab", NULL };

#define	addmode	strncat(modes, space, sizeof(modes)); \
		space = " "; \
		snprintf(modes+strlen(modes), sizeof(modes)-strlen(modes),

void
main(int ac, char **av)
{
	char *space = "";
	char *ms = NULL;
	char *m2 = NULL;
	char modes[1024];
	char errs[8192];
	char *b, *c;

	modes[0] = '\0';
	errs[0] = '\0';

	if (ac != 2) {
		fprintf(stderr, "usage: %s entry\n", av[0]);
		exit(1);
	}

	if (cgetent(&b, gettytab, av[1]) != 0) {
		fprintf(stderr, "%s: could not find valid entry\n", av[1]);
		exit(1);
	}
	c = strchr(b, ':');	/* skip name entries */
	printf("%s:", av[1]);
	if (c == NULL)
		exit(0);
	while (b = c) {
		if ((c = strchr(++b, ':')) == NULL)
			break;
		while (*b == ' ' || *b == '\t')
			++b;
		if (b == c)
			continue;
		*c = '\0';
		if (strncmp(b, "sp#", 3) == 0)
			printf("\\\n\t:dte=%s:", b+3);
		else if (strncmp(b, "im=", 3) == 0)
			printf("\\\n\t:banner=%s:", b+3);
		else if (strncmp(b, "nx=", 3) == 0) {
			strncat(errs, "warning, autobauding not supported: ",
			    sizeof(errs));
			strncat(errs, b, sizeof(errs));
			strncat(errs, "\n", sizeof(errs));
		} else if (strncmp(b, "lm=", 3) == 0) {
			strncat(errs,
			    "warning, login message cannot be changed: ",
			    sizeof(errs));
			strncat(errs, b, sizeof(errs));
			strncat(errs, "\n", sizeof(errs));
		} else if (strcmp(b, "rw") == 0) {
			strncat(errs, "warning, rawmode (rw) not supported\n",
			    sizeof(errs));
		} else if (strncmp(b, "pf#", 3) == 0)
			;
		else if (strncmp(b, "ms=", 3) == 0) {
			if (ms == NULL)
				ms = strdup(b+3);
		} else if (strncmp(b, "m2=", 3) == 0) {
			if (ms == NULL)
				m2 = strdup(b+3);
		} else if (strncmp(b, "de#", 3) == 0)
			;
		else if (strncmp(b, "pc=", 3) == 0)
			;
		else if (strcmp(b, "bi") == 0)
			printf("\\\n\t:dialin:dialout:");
		else if (strcmp(b, "hw") == 0) {
			addmode "clocal");
		} else if (strcmp(b, "hf") == 0) {
			addmode "cts_oflow rts_iflow");
		} else if (strcmp(b, "np") == 0) {
			addmode "-parenb -parodd");
		} else if (strcmp(b, "ep") == 0) {
			addmode "parenb -parodd");
		} else if (strcmp(b, "op") == 0) {
			addmode "parenb parodd");
		} else if (strcmp(b, "ht") == 0) {
			addmode "-oxtabs");
		} else if (strcmp(b, "nl") == 0) {
			addmode "icrnl onlcr");
		} else if (strcmp(b, "ap") == 0) {
			addmode "cs7 -parenb");
		} else if (strcmp(b, "ec") == 0) {
			addmode "-echo");
		} else if (strcmp(b, "hc") == 0) {
			addmode "-hupcl");
		} else if (strcmp(b, "pe") == 0) {
			addmode "echoprt");
		} else if (strcmp(b, "dx") == 0) {
			addmode "-ixany");
		} else if (strcmp(b, "ce") == 0) {
			addmode "echoe");
		} else if (strcmp(b, "ck") == 0) {
			addmode "echok");
		} else if (strncmp(b, "is#", 3) == 0) {
			addmode "ispeed %s", b+3);
		} else if (strncmp(b, "os#", 3) == 0) {
			addmode "ospeed %s", b+3);
		} else if (strncmp(b, "bk=", 3) == 0) {
			addmode "eol2 %s", b+3);
		} else if (strncmp(b, "ds=", 3) == 0) {
			addmode "dsusp %s", b+3);
		} else if (strncmp(b, "er=", 3) == 0) {
			addmode "erase %s", b+3);
		} else if (strncmp(b, "et=", 3) == 0) {
			addmode "eof %s", b+3);
		} else if (strncmp(b, "fl=", 3) == 0) {
			addmode "discard %s", b+3);
		} else if (strncmp(b, "in=", 3) == 0) {
			addmode "intr %s", b+3);
		} else if (strncmp(b, "kl=", 3) == 0) {
			addmode "kill %s", b+3);
		} else if (strncmp(b, "ln=", 3) == 0) {
			addmode "lnext %s", b+3);
		} else if (strncmp(b, "qu=", 3) == 0) {
			addmode "quit %s", b+3);
		} else if (strncmp(b, "su=", 3) == 0) {
			addmode "susp %s", b+3);
		} else {
			strncat(errs, b, sizeof(errs));
			strncat(errs, ": cannot automatically convert\n",
			    sizeof(errs));
		}
	}
	space = "";
	if (*modes || ms || m2)	printf("\\\n\t:stty-modes=");
	if (ms) {printf("%s%s", space, ms); space = " "; }
	if (*modes) {printf("%s%s", space, modes); space = " "; }
	if (m2) {printf("%s%s", space, m2); space = " "; }
	if (*modes || ms || m2)	printf(":");
	printf("\n");

	if (errs)
		fprintf(stderr, "%s", errs);
	exit(0);
}
