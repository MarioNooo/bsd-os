/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI keymap.c,v 1.1 1997/01/02 17:38:36 pjd Exp
 *
 */

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include <i386/isa/pcconsioctl.h>
#include <i386/isa/pcconsvar.h>

#include <stdio.h>
#include <err.h>
#include <paths.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#define	_PATH_CONSTABS "/usr/libdata/keyboard/"

int	readtab __P((struct constab *, char *));
char	*actionName __P((int));
static void	keymap_usage __P((void));
void	keymap_help __P((void));

int
keymap(argc, argv, fd)
	int argc, fd;
	char *argv[];
{
	int	reset = 0;
	struct constab ct;

        if (strcmp(argv[0], "reset") == 0)
        	reset++;

        if (argc > 1) {
		keymap_usage();
                return (1);
        }


	/* init constab */
	ct.ct_constabsize = 0;	/* size of standard mapping table */
	ct.ct_extstart = 0;	/* first extended scan code */
	ct.ct_extend = 0;	/* last extended scan code */
	ct.ct_altconstabsize = 0; /* size of alternate mapping table */
	ct.ct_maxaccent = 0;	/* number of different accents */
	ct.ct_accenttabsize = 0; /* number of different accents */
	ct.ct_constab = NULL;	/* pointer to contabs buffers */

	if (reset == 0) {
		if (readtab(&ct, argv[argc-1]))
			errx(1, "error parsing keymap table ... not loaded!");
	}

	if (ioctl(fd, PCCONIOCSCONSTAB, &ct) < 0)
		err(1, "set console keyboard map");

	return(0);
}

int
readtab(struct constab *ct, char *name)
{
	FILE	*fp;
	int	i, n, line;
	int	cts, ets, ete, ats, mac, acc;
	int	size;
	int	ac, c0, c1, c2, c3, c4, c5, c6, c7;
	static char fnm[1024];
	static char buf[1024];
	char	*p;
	struct key *kb;

#define	S_LINE1 0
#define	S_LINE2 1
#define	S_CNS	2
#define	S_EXT	3
#define	S_ALT	4
#define	S_ACC	5
#define	S_EOF	6

	int	state = S_LINE1;

	if ((fp = fopen(name, "r")) == NULL) {
		strcpy(fnm, _PATH_CONSTABS);
		strncat(fnm, name, sizeof(fnm)-strlen(fnm)-2);
		fnm[sizeof(fnm)-1] = '\0';

		if ((fp = fopen(fnm, "r")) == NULL)
			err(1, "%s", name);
	}

	line = 0;

	while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
		line++;

		if ((p = strchr(buf, '\n')) != NULL)
			*p = '\0';
		if ((p = strchr(buf, '#')) != NULL)
			*p = '\0';

		p = buf;
		while (*p && isspace(*p))
			p++;

		if (*p) {
			if (state == S_LINE1) {
				state = S_LINE2;
			} else if (state == S_LINE2) {
				if ((n = sscanf(p, "%d %d %d %d %d %d", 
				  &cts, &ets, &ete, &ats, &mac, &acc)) != 6) {
					fprintf(stderr, "error: %s/%d: second data line format (found %d numbers, need 6)n", name, line, n);
					return (1);
				}

				size = sizeof(struct key) * (cts + (ete-ets+1)
				  + ats + (mac+1) * acc);


				if ((kb = (struct key *) malloc(size)) == NULL) {
					fprintf(stderr, "can't malloc buffer\n");
					return (1);
				}
				ct->ct_constabsize = cts;
				ct->ct_extstart = ets;
				ct->ct_extend = ete;
				ct->ct_altconstabsize = ats;
				ct->ct_maxaccent = mac;
				ct->ct_accenttabsize = acc;
				ct->ct_constab = kb;
				state = S_CNS;
				i = 0;
			} else if (state == S_CNS || state == S_EXT || 
			  state == S_ALT || state == S_ACC) {
				if ((n = sscanf(p, "%i %i %i %i %i %i %i %i %i",
				  &ac, &c0, &c1, &c2, &c3, &c4, &c5, &c6, &c7))
				  != 9) {
					fprintf(stderr, "error: %s/%d: key table data line format (found %d items, need 9)\n", name, line, n);
					return (1);
				}
				if (ac < IGNORE || ac > SFUNC) {
  					fprintf(stderr, "error: %s/%d: action out of range (%d)\n", name, line, ac);
					return (1);
				}

				kb->action = ac;
				kb->code[0] = c0;
				kb->code[1] = c1;
				kb->code[2] = c2;
				kb->code[3] = c3;
				kb->code[4] = c4;
				kb->code[5] = c5;
				kb->code[6] = c6;
				kb->code[7] = c7;
				kb++;
				i++;

				switch (state) {
				case S_CNS:
					if (i >= cts) {
						i = 0;
						state = S_EXT;
					}
					break;
				case S_EXT:
					if (i >= ete - ets + 1) {
						i = 0;
						state = S_ALT;
					}
					break;
				case S_ALT:
					if (i >= ats) {
						i = 0;
						state = S_ACC;
					}
					break;
				case S_ACC:
					if (i >= (mac+1) * acc) {
						i = 0;
						state = S_EOF;
					}
					break;
				default:
					fprintf(stderr, "oops ... case botched (%d)\n", state);
					return (1);
				}
			} else if (state == S_EOF) {
				fprintf(stderr, "error: %s/%d: garbage after end of tables: `%s' ?\n", name, line, p);
				return (1);
			} else {
				fprintf(stderr, "oops ... state botched (%d)\n", state);
				return (1);
			}
		}
	}

	if (state == S_EOF || (state == S_ACC && mac == 0 && acc == 0))
		return (0);

	fprintf(stderr, "error: %s/%d: file incomplete, need %s line (state %d)\n", name, line,
	  state == S_LINE1 ? "first header" :
	  state == S_LINE2 ? "second header" :
	  state == S_CNS ? "constab" :
	  state == S_EXT ? "exttab" :
	  state == S_ALT ? "altconstab" :
	  state == S_ACC ? "accenttab" : "<unknown>",
	  state);
	return (1);
}

char *actiontab[] = {
  /*  0 */ "Ignore",
  /*  1 */ "Shift",
  /*  2 */ "Shift (r)",
  /*  3 */ "Ctrl",
  /*  4 */ "Ctrl (r)",
  /*  5 */ "Alt",
  /*  6 */ "Alt (r)",
  /*  7 */ "AltShift",
  /*  8 */ "Accent",
  /*  9 */ "CapsLock",
  /* 10 */ "NumLock",
  /* 11 */ "DebgFunc",
  /* 12 */ "ScrLock",
  /* 13 */ "Char",
  /* 14 */ "Alpha",
  /* 15 */ "Func",
  /* 16 */ "NumPad",
  /* 17 */ "Accented",
  /* 18 */ "ShftFunc"
};

char *
actionName(ac)
int ac;
{
	static char buf[64];

	if (ac < IGNORE || ac > SFUNC) {
		sprintf(buf, "<illegal value %d>", ac);
		return (buf);
	}

	return (actiontab[ac]);
}

void
keymap_help()
{
        printf("keymap category:\n");
        printf("\t   reset -- load default keymap\n");
        printf("\tfilename -- load keymap from filename\n");
}

static void
keymap_usage()
{
        fprintf(stderr, "usage: setcons keymap reset\n");
        fprintf(stderr,
            "usage: setcons keymap filename\n");
}

