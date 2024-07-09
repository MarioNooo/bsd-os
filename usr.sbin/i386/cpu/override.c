/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	override.c,v 1.7 2003/09/16 01:40:47 jch Exp
 */
#include "cpu.h"
#include <regex.h>
#include <ctype.h>

/*
 * List of all overrides
 */

static char *ovr_all[] = {
	ovr_nmi,
	ovr_nmidest,
	ovr_nonmi,
	ovr_smi,
	ovr_smidest,
	ovr_nosmi,
	ovr_cpu,
	ovr_irq,
	ovr_noext,
	ovr_noextclk,
	ovr_extclk,
	ovr_extdest,
	ovr_apic,
	ovr_cpuboot,
	ovr_extint,
	ovr_apic,
	ovr_nopci,
	ovr_noisa,
	ovr_pccard,
	ovr_notpr,
	NULL
};

extern char *ovr_file;

static void ovr_start __P((char **av, int ac, char **valid));

/*
 * Set up overrides, we try to open the override file and parse it, and then
 * we tack on the command line args.
 */
void
setup_overrides(char **av, int ac)
{
	char *ovr[MAX_OVR];
	int novr;
	FILE *fp;
	char buf[128];
	char *in;
	char *out;
	char c;
	char **ovr_p;
	char **tp;

	novr = 0;
	if (ovr_file != NULL) {
		if (access(ovr_file, O_RDONLY) < 0) {
			if (errno != ENOENT)
				err(1, "Can't read %s", ovr_file);
		} else {
			fp = fopen(ovr_file, "r");
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				/* Nuke whitespace and comments */
				in = out = buf;
				do {
					c = *out++;
					if (isspace(c))
						continue;
					if (c == '#')
						c = '\0';
					*in++ = c;
				} while (c != '\0');
				if (in == &buf[1])
					continue;
				if (novr == MAX_OVR)
					errx(1, "Too many configuration "
					    "overrides");
				ovr[novr++] = strdup(buf);
			}
			fclose(fp);
		}
	}
	while (ac--) {
		if (novr == MAX_OVR)
			errx(1, "Too many configuration overrides");
		ovr[novr++] = *av++;
	}
	ovr_p = malloc(sizeof(char *) * novr);
	bcopy(ovr, ovr_p, novr * sizeof(char *));
	ovr_start(ovr_p, novr, ovr_all);
		
	if (vflag && novr) {
		printf("Configuration overrides:\n");
		for (tp = ovr_p; tp != &ovr_p[novr]; tp++)
			printf("\t%s\n", *tp);
	}
}

/*
 * Override parsing and support routines.
 *
 * General form of an override is a single argument with a key seperated
 * from an optional argument by an '='. Ex:
 *	key
 *	key=arg
 *
 * Different routines are supplied to handle/parse several formats for 'arg':
 *	key=number
 *	key=number,number
 *	key=something,flag,something	(check for presence of flag)
 *
 * All override parsing is done via a pointer to a char * array and a count,
 * the ovr_check() funtion pre-parses and caches the overrides so the 
 * argv/argc are not needed on future calls (just the cookie returned by
 * by ovr_check()).
 */

static char **oav;
static int oac;

/*
 * Pre-parse and check overrides, must be called before working with any
 * group of overrides (and only 1 group can be worked with a at time, this
 * for simplicity in the calls).
 *
 * Valid gives a regexp that is compared against each entries in the av
 * array (use the '|' construct to check for various keywords).
 * If valid == NULL no checks are performed.
 */
static void
ovr_start(char **av, int ac, char **valid)
{
	regex_t prog;
	int rc;
	char buf[80];
	int i;
	char **ap;

	oav = av;
	oac = ac;
	for (i = 0; i < ac; i++) {
		for (ap = valid; *ap != NULL; ap++) {
			if ((rc = regcomp(&prog, *ap,
			    REG_EXTENDED | REG_NOSUB)) != 0) {
				regerror(rc, &prog, buf, sizeof(buf));
				errx(1, "Error in regexp: %s", buf);
			}
			if (regexec(&prog, av[i], 0, NULL, 0) == 0)
				break;
		}
		if (*ap == NULL)
			errx(1, "Override error: %s", av[i]);
	}
}

/*
 * Check if a particular override flag exists
 */
int
ovr(char *key)
{
	char **ap;
	regex_t prog;
	int rc;
	char nbuf[64];

	if ((rc = regcomp(&prog, key, REG_EXTENDED | REG_NOSUB)) != 0) {
		regerror(rc, &prog, nbuf, sizeof(nbuf));
		errx(1, "Error in regexp \"%s\": %s", key, nbuf);
	}

	for (ap = oav; ap < &oav[oac]; ap++)
		if (regexec(&prog, *ap, 0, NULL, 0) == 0)
			break;
	return (ap != &oav[oac]);
}

/*
 * Check for override with numeric arguments.
 *
 * Argument passed is a regexp, if matched each substring is passed to
 * strtoul(matchstring, NULL, 0) and the result stored in nums[X]. Be
 * careful to provide enough space in the nums array for the number of
 * substrings defined in the regexp.
 *
 * The which argument defines which matching entry is return (0 == first one,
 * 1 == second, etc...).
 */
char *
ovrn(char *re, int which, int *nums)
{
	char **ap;
	regex_t prog;
	int rc;
	regmatch_t mats[10];
	char nbuf[64];
	int i;
	int len;

	if ((rc = regcomp(&prog, re, REG_EXTENDED)) != 0) {
		regerror(rc, &prog, nbuf, sizeof(nbuf));
		errx(1, "Error in regexp: %s", nbuf);
	}
	for (ap = oav; ap < &oav[oac]; ap++) {
		if ((regexec(&prog, *ap, 10, mats, 0) == 0) && (which-- == 0))
			break;
	}
	if (ap == &oav[oac])
		return (NULL);
	if (nums != NULL) {
		for (i = 1; i < 10; i++) {
			if (mats[i].rm_so == -1)
				continue;
			len = mats[i].rm_eo - mats[i].rm_so;
			strncpy(nbuf, (*ap) + mats[i].rm_so, len);
			nbuf[len] = '\0';
			nums[i - 1] = (int)strtoul(nbuf, NULL, 0);
		}
	}
	return (*ap);
}

/*
 * Check a regular expression against a string (simple stuff)
 */
int
ovr_chk(char *re, char *str)
{
	regex_t prog;
	int rc;
	char nbuf[64];

	if ((rc = regcomp(&prog, re, REG_EXTENDED | REG_NOSUB)) != 0) {
		regerror(rc, &prog, nbuf, sizeof(nbuf));
		errx(1, "Error in regexp \"%s\": %s", re, nbuf);
	}
	return (regexec(&prog, str, 0, NULL, 0) == 0);
}


/* ----------------------------------------------------------------------- */

/* char extint[] = "extint=([.0x.]?[:isxdigit:]+),([.0x.]?[:isxdigit:]+)"; */
char extint[] = "extint=" RNUM "(," RNUM "){0,1}";
char irq[] = "^irq" RNUM "=" RNUM "," RNUM "(,(level|excl){1,1})*$";

char *tval[] = { "nopci", "noext", "noextclk", "extclk", extint, irq, NULL };

/*
 * Test overrides
 */
int
test_ovr(int ac, char **av)
{
	int num[10];
	int i;
	int j;
	char *key;

	ovr_start(&av[1], ac - 1, tval);
	i = 0;
	for (;;) {
		for (j = 0; j < 10; j++)
			num[j] = -1;
		if ((key = ovrn(irq, i, num)) == NULL)
			break;
		printf("Key=%s\nInstance %d, 0=%d 1=%d 2=%d 3=%d 4=%d 5=%d\n",
		    key,
		    i, num[0], num[1], num[2], num[3], num[4], num[5]);
		if (ovr_chk("level", key)) {
			printf("\tLevel\n");
		}
		if (ovr_chk("excl", key)) {
			printf("\tExcl\n");
		}
		i++;
	}
	return (0);
}
