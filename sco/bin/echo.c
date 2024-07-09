/*
 * Copyright (c) 1995 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI echo.c,v 2.1 1995/02/03 15:12:58 polk Exp
 */

/*
 * System V compatible /bin/echo.
 * Based on POSIX.2 rationale (p. 900).
 */

#include <stdio.h>

char **xargv;
int nflag;

int
egetc()
{
	static char *p;

	if (*xargv == 0)
		return (EOF);
	if (p == 0)
		p = *xargv;
	if (*p)
		return (*p++);
	p = 0;
	if (*++xargv)
		return (' ');
	return (EOF);
}

void
process()
{
	int c;
	int i;
	int v;
	int pushback = EOF;

	while ((c = pushback) != EOF || (c = egetc()) != EOF) {
		pushback = EOF;
		if (c != '\\') {
			putchar(c);
			continue;
		}
		switch (c = egetc()) {
		case 'a':
			putchar('\a');
			break;
		case 'b':
			putchar('\b');
			break;
		case 'c':
			return;
		case 'f':
			putchar('\f');
			break;
		case 'n':
			putchar('\n');
			break;
		case 'r':
			putchar('\r');
			break;
		case 't':
			putchar('\t');
			break;
		case 'v':
			putchar('\v');
			break;
		case '\\':
			putchar('\\');
			break;
		case '0':
			i = 0;
			v = 0;
			while ((c = egetc()) != EOF && i++ < 3 &&
			    c >= '0' && c <= '7')
				v = (c - '0') + v * 8;
			putchar(v);
			pushback = c;
			break;
		case EOF:
			putchar('\\');
			return;
		default:
			putchar('\\');
			putchar(c);
			break;
		}
	}
	if (!nflag)
		putchar('\n');
}

int
main(argc, argv)
	int argc;
	char **argv;
{

	if (argc > 1 && strcmp(argv[1], "-n") == 0) {
		++nflag;
		++argv;
	}

	xargv = ++argv;
	process();
	return (0);
}
