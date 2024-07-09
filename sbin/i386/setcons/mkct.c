/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI mkct.c,v 1.1 1997/01/02 17:38:37 pjd Exp
 *
 */
/*
 * mkct.c
 *
 * create loadable constab files for setcons
 *
 * 951105 B. Schneck, GeNUA
 *        initial version
 */
#include <sys/types.h>

#include <i386/isa/pcconsvar.h>
#include <i386/isa/pcconsioctl.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef	KBD
#error WHICH KEYBOARD DID YOU WANT?
#endif

/*
 * Macro magic to #include KBDPREF ## KBD
 *
 * stolen form i386/i386/i386_param.c
 */
#undef i386
#ifdef __STDC__ 
#define KBDPREF	/sys/i386/isa/kbd/pcconstab.
#define	R(a,b)	< ## a ## b ## >
#define	S(a,b)	R(a,b)

#include S(KBDPREF,KBD)

#define	Q(a)	""#a""
#define QS(a)	Q(a)

#else /* __STDC__ */

#define KBDPREF	/sys/i386/isa/kbd/pcconstab
#define	D	KBDPREF.KBD
#define	I	<D>
#include I
#endif /* __STDC__ */
#define	i386

static void mkct_usage __P(());

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *fp;
	int ch, i, j, accenttabsize = 0, numaccent = 0;

	while ((ch = getopt(argc, argv, "")) != -1)
		switch (ch) {
		case '?':
		default:
			mkct_usage();
		}
	argc -= optind;
	argv += optind;

	if ((fp = fopen(QS(KBD), "w")) == NULL)
		err(1, "QS(KB)");

	for (i = 0; i < pcconstabsize; i++)
		if (pcconstab[i].action == ACCENTED)
			numaccent++;

	for (i = 0; i < pcextend - pcextstart + 1; i++)
		if (pcexttab[i].action == ACCENTED)
			numaccent++;

	for (i = 0; i < pcaltconstabsize; i++)
		if (pcaltconstab[i].action == ACCENTED)
			numaccent++;

	accenttabsize = sizeof(accenttab) / sizeof(accenttab[0]);

	fprintf(fp, "standard %s keyboard\n", QS(KBD));
	fprintf(fp, "%d %d %d %d %d %d\n", pcconstabsize,
	    pcextstart, pcextend, pcaltconstabsize, maxaccent, numaccent);

	fprintf(fp, "# pcconstab (%d)\n", pcconstabsize);

	for (i = 0; i < pcconstabsize; i++) {
		if ((j = fprintf(fp,
	"%3d 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
		  pcconstab[i].action,
		  pcconstab[i].code[0],
		  pcconstab[i].code[1],
		  pcconstab[i].code[2],
		  pcconstab[i].code[3],
		  pcconstab[i].code[4],
		  pcconstab[i].code[5],
		  pcconstab[i].code[6],
		  pcconstab[i].code[7])) <= 0)
			err(1, "fprintf %d", j);

		for (j = 0; j < 8; j++) {
			switch(pcconstab[i].action) {
			case ALTSHIFT:
				if (pcconstab[i].code[j] >= accenttabsize)
					errx(1,
	"invalid code - pcconstab[%d].code[%d](%d) >= accenttabsize(%d)",
			i, j, pcconstab[i].code[j], accenttabsize);
				break;
			case ACCENTED:
				if (pcconstab[i].code[j] >= (accenttabsize +
				  maxaccent))
					errx(1,
 "invalid code - pcconstab[%d].code[%d](%d) >= accenttabsize + maxaccent (%d)",
		i, j, pcconstab[i].code[j], (maxaccent + accenttabsize));
				break;
			default:
				break;
			}
		}
	}

	fprintf(fp, "# pcexttab (%d - %d)\n", pcextstart, pcextend);

	for (i = 0; i < pcextend - pcextstart + 1; i++) {
		if ((j = fprintf(fp,
	"%3d 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
		  pcexttab[i].action,
		  pcexttab[i].code[0],
		  pcexttab[i].code[1],
		  pcexttab[i].code[2],
		  pcexttab[i].code[3],
		  pcexttab[i].code[4],
		  pcexttab[i].code[5],
		  pcexttab[i].code[6],
		  pcexttab[i].code[7])) <= 0)
			err(1, "fprintf %d", j);

		for (j = 0; j < 8; j++) {
			switch(pcexttab[i].action) {
			case ALTSHIFT:
				if (pcexttab[i].code[j] >= accenttabsize)
					errx(1,
	"invalid code - pcexttab[%d].code[%d](%d) >= accenttabsize(%d)",
			i, j, pcexttab[i].code[j], accenttabsize);
				break;
			case ACCENTED:
				if (pcexttab[i].code[j] >= (accenttabsize +
				  maxaccent))
					errx(1,
 "invalid code - pcexttab[%d].code[%d](%d) >= accenttabsize + maxaccent(%d)",
		i, j, pcexttab[i].code[j], (maxaccent + accenttabsize));
				break;
			default:
				break;
			}
		}
	}

	fprintf(fp, "# pcaltconstab (%d)\n", pcaltconstabsize);

	for (i = 0; i < pcaltconstabsize; i++) {
		if ((j = fprintf(fp,
	"%3d 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
		  pcaltconstab[i].action,
		  pcaltconstab[i].code[0],
		  pcaltconstab[i].code[1],
		  pcaltconstab[i].code[2],
		  pcaltconstab[i].code[3],
		  pcaltconstab[i].code[4],
		  pcaltconstab[i].code[5],
		  pcaltconstab[i].code[6],
		  pcaltconstab[i].code[7])) <= 0)
			err(1, "fprintf %d", j);

		for (j = 0; j < 8; j++) {
			switch(pcaltconstab[i].action) {
			case ALTSHIFT:
				if (pcaltconstab[i].code[j] >= accenttabsize)
					errx(1,
	"invalid code - pcaltconstab[%d].code[%d](%d) >= accenttabsize(%d)",
		i, j, pcaltconstab[i].code[j], accenttabsize);
				break;
			case ACCENTED:
				if (pcaltconstab[i].code[j] >= (accenttabsize +
				  maxaccent))
					errx(1,
"invalid code - pcaltconstab[%d].code[%d](%d) >= accenttabsize + maxaccent(%d)",
		i, j, pcaltconstab[i].code[j], (maxaccent + accenttabsize));
				break;
			default:
				break;
			}
		}
	}

	fprintf(fp,
	    "# accenttab (%d accented chars, base char + %d accents)\n",
	    numaccent, maxaccent);

	for (i = 0; i < numaccent * (maxaccent + 1); i++) {
		if ((j = fprintf(fp,
	"%3d 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
		  accenttab[i].action,
		  accenttab[i].code[0],
		  accenttab[i].code[1],
		  accenttab[i].code[2],
		  accenttab[i].code[3],
		  accenttab[i].code[4],
		  accenttab[i].code[5],
		  accenttab[i].code[6],
		  accenttab[i].code[7])) <= 0)
			err(1, "fprintf %d", j);

		for (j = 0; j < 8; j++) {
			switch(accenttab[i].action) {
			case ALTSHIFT:
				if (accenttab[i].code[j] >= accenttabsize)
					errx(1,
	"invalid code - accenttab[%d].code[%d](%d) >= accenttabsize(%d)",
		i, j, accenttab[i].code[j], accenttabsize);
				break;
			case ACCENTED:
				if (accenttab[i].code[j] >= (accenttabsize +
				  maxaccent))
					errx(1,
 "invalid code - accenttab[%d].code[%d](%d) >= accenttabsize + maxaccent(%d)",
		i, j, accenttab[i].code[j], (maxaccent + accenttabsize));
				break;
			default:
				break;
			}
		}
	}

	exit(0);
}

static void
mkct_usage()
{
        fprintf(stderr, "usage: mkct\n");
}

