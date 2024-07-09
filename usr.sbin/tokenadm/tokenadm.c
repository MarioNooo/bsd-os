/*	BSDI tokenadm.c,v 1.3 2001/05/31 17:28:38 prb Exp */
/*-
//////////////////////////////////////////////////////////////////////////
//									//
//   Copyright (c) 1995 Migration Associates Corp. All Rights Reserved	//
//									//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MIGRATION ASSOCIATES	//
//	The copyright notice above does not evidence any   		//
//	actual or intended publication of such source code.		//
//									//
//	Use, duplication, or disclosure by the Government is		//
//	subject to restrictions as set forth in FAR 52.227-19,		//
//	and (for NASA) as supplemented in NASA FAR 18.52.227-19,	//
//	in subparagraph (c) (1) (ii) of Rights in Technical Data	//
//	and Computer Software clause at DFARS 252.227-7013, any		//
//	successor regulations or comparable regulations of other	//
//	Government agencies as appropriate.				//
//									//
//		Migration Associates Corporation			//
//			19935 Hamal Drive				//
//			Monument, CO 80132				//
//									//
//	A license is granted to Berkeley Software Design, Inc. by	//
//	Migration Associates Corporation to redistribute this software	//
//	under the terms and conditions of the software License		//
//	Agreement provided with this distribution.  The Berkeley	//
//	Software Design Inc. software License Agreement specifies the	//
//	terms and conditions for redistribution.			//
//									//
//	UNDER U.S. LAW, THIS SOFTWARE MAY REQUIRE A LICENSE FROM	//
//	THE U.S. COMMERCE DEPARTMENT TO BE EXPORTED.  IT IS YOUR	//
//	REQUIREMENT TO OBTAIN THIS LICENSE PRIOR TO EXPORTATION.	//
//									//
//////////////////////////////////////////////////////////////////////////
*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>

#include "token.h"
#include "tokendb.h"


typedef enum { LIST, ENABLE, DISABLE, REMOVE, MODECH } what_t;
typedef enum {
	NOBANNER = 0x01,
	TERSE = 0x02,
	ENONLY = 0x04,
	DISONLY = 0x08,
	ONECOL = 0x10,
	REVERSE = 0x20,
	} how_t;

static	int	force_unlock(char *);
static	int	process_record(char *, unsigned, unsigned);
static	int	process_modes(char *, unsigned, unsigned);
static  void	print_record(TOKENDB_Rec *, how_t);

extern	int
main(int argc, char **argv)
{
	char *m;
	int c, d, errors;
	u_int emode, dmode, pmode;
	struct rlimit cds;
	what_t what;
	how_t how;
	TOKENDB_Rec tokenrec;

	what = LIST;
	emode = dmode = 0;
	pmode = 0;
	errors = 0;
	how = 0;

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog(NULL, LOG_ODELAY, LOG_AUTH);

	if (token_init(argv[0]) < 0) {
		syslog(LOG_ERR, "unknown token type");
		errx(1, "unknown token type");
	}

	/*
	 * Make sure we never dump core as we might have a
	 * valid user shared-secret in memory.
	 */

	cds.rlim_cur = 0;
	cds.rlim_max = 0;
	if (setrlimit(RLIMIT_CORE, &cds) < 0)
		syslog(LOG_ERR, "couldn't set core dump size to 0: %m");

    	while ((c = getopt(argc, argv, "BDERT1bdem:ru")) != EOF)
		switch (c) {
		case 'B':
			if (what != LIST)
				goto usage;
			how |= NOBANNER;
			break;
		case 'T':
			if (what != LIST)
				goto usage;
			how |= TERSE;
			break;
		case '1':
			if (what != LIST)
				goto usage;
			how |= ONECOL;
			break;
		case 'D':
			if (what != LIST)
				goto usage;
			how |= DISONLY;
			break;
		case 'E':
			if (what != LIST)
				goto usage;
			how |= ENONLY;
			break;
		case 'R':
			if (what != LIST)
				goto usage;
			how |= REVERSE;
			break;
		case 'd':
			if (what != LIST || how)
				goto usage;
			what = DISABLE;
			break;
		case 'e':
			if (what != LIST || how)
				goto usage;
			what = ENABLE;
			break;
		case 'r':
			if (what != LIST || emode || dmode || how)
				goto usage;
			what = REMOVE;
			break;
		case 'm':
			if (what == REMOVE || how)
				goto usage;
			if (*optarg == '-') {
				if ((c = token_mode(optarg+1)) == NULL)
					errx(1, "%s: unknown mode", optarg+1);
				dmode |= c;
			} else {
				if ((c = token_mode(optarg)) == NULL)
					errx(1, "%s: unknown mode", optarg);
				emode |= c;
			}
			break;
		default:
			goto usage;
		}

	if (what == LIST && (dmode || emode))
		what = MODECH;

	if (what == LIST) {
		if ((how & (ENONLY|DISONLY)) == 0)
			how |= ENONLY|DISONLY;
		if (!(how & NOBANNER)) {
			if ((how & (TERSE|ONECOL)) == (TERSE|ONECOL)) {
				printf("User\n");
				printf("----------------\n");
			} else if (how & (TERSE)) {
				printf("User             ");
				printf("User             ");
				printf("User             ");
				printf("User\n");
				printf("---------------- ");
				printf("---------------- ");
				printf("---------------- ");
				printf("----------------\n");
			} else {
				printf("User             Status   Modes\n");
				printf("---------------- -------- -----\n");
			}
		}

		if (optind >= argc) {
			if (tokendb_firstrec(how & REVERSE, &tokenrec))
				exit(0);
			do
				print_record(&tokenrec, how);
			while (tokendb_nextrec(how & REVERSE, &tokenrec) == 0);
			print_record(NULL, how);
			exit(0);
		}
	}

	if (optind >= argc) {
usage:
		fprintf(stderr,
		    "Usage: %sadm [-BDERT1 | -d | -e | -r] [-m mode] user [...]\n",
			tt->name);
		exit(1);
	}
		

	argv += optind - 1;
	while (*++argv)
		switch(what) {
		case LIST:
			if (tokendb_getrec(*argv, &tokenrec)) {
				printf("%s: no such user\n");
				break;
			}
			print_record(&tokenrec, how);
			break;
		case REMOVE:
			if (tokendb_delrec(*argv)) {
				warnx("%s: could not remove", *argv);
				errors++;
			}
			break;
		case DISABLE:
			if (process_record(*argv, ~TOKEN_ENABLED, 0)) {
				warnx("%s: could not disable", *argv);
				++errors;
			}
			if (emode || dmode)
				goto modech;
			break;
		case ENABLE:
			if (process_record(*argv, ~TOKEN_ENABLED, TOKEN_ENABLED)) {
				warnx("%s: could not enable", *argv);
				++errors;
			}
			if (emode || dmode)
				goto modech;
			break;
		modech:
		case MODECH:
			if (process_modes(*argv, ~dmode, emode)) {
				warnx("%s: could not change modes", *argv);
				++errors;
			}
			break;
		}

	if (what == LIST)
		print_record(NULL, how);

	exit(errors);
}

/*
 * Process a user record
 */

static	int
process_record(char *username, unsigned and_mask, unsigned or_mask)
{
	int	count = 0;
	TOKENDB_Rec tokenrec;

retry:
	switch (tokendb_lockrec(username, &tokenrec, TOKEN_LOCKED)) {
	case 0:
		tokenrec.flags &= and_mask;
		tokenrec.flags |= or_mask;
		tokenrec.flags &= ~TOKEN_LOCKED;
		if (!tokendb_putrec(username, &tokenrec))
			return (0);
		else
			return (-1);
	case 1:
		sleep(1);
		if (count++ < 60)
			goto retry;
		if (force_unlock(username))
			return (1);
		goto retry;

	case ENOENT:
		warnx("%s: nonexistent user", username);
		return (1);
	default:
		return (-1);
	}
}

static	int
process_modes(char *username, unsigned and_mask, unsigned or_mask)
{
	int	count = 0;
	TOKENDB_Rec tokenrec;

retry:
	switch (tokendb_lockrec(username, &tokenrec, TOKEN_LOCKED)) {
	case 0:
		tokenrec.mode &= and_mask;
		tokenrec.mode |= or_mask;
		/*
		 * When ever we set up for rim mode (even if we are
		 * already set up for it) reset the rim key
		 */
		if (or_mask & TOKEN_RIM)
			memset(tokenrec.rim, 0, sizeof(tokenrec.rim));
		tokenrec.flags &= ~TOKEN_LOCKED;
		if (!tokendb_putrec(username, &tokenrec))
			return (0);
		else
			return (-1);
	case 1:
		sleep(1);
		if (count++ < 60)
			goto retry;
		if (force_unlock(username))
			return (1);
		goto retry;

	case ENOENT:
		warnx("%s: nonexistent user", username);
		return (1);
	default:
		return (-1);
	}
}

/*
 * Force remove a user record-level lock.  
 */

static	int
force_unlock(char *username)
{
	TOKENDB_Rec tokenrec;

	if (tokendb_getrec(username, &tokenrec))
		return (-1);

	tokenrec.flags &= ~TOKEN_LOCKED;
	tokenrec.flags &= ~TOKEN_LOGIN;

	if (tokendb_putrec(username, &tokenrec))
		return (1);

	return (0);
}

/*
 * Print a database record according to user a specified format
 */

static	void
print_record(TOKENDB_Rec *rec, how_t how)
{
	static int count = 0;
	int i;

	if (rec == NULL) {
		if ((count & 3) && (how & (TERSE|ONECOL)) == TERSE)
			printf("\n");
		return;
	}

	if (rec->flags & TOKEN_ENABLED) {
		if ((how & ENONLY) == 0)
			return;
	} else {
		if ((how & DISONLY) == 0)
			return;
	}

	switch (how & (TERSE|ONECOL)) {
	case 0:
	case ONECOL:
		printf("%-16s %-8s", rec->uname,
		  rec->flags & TOKEN_ENABLED ? "enabled" : "disabled");

		for (i = 1; i; i <<= 1)
			if (rec->mode & i)
				printf(" %s", token_getmode(i));
		printf("\n");
		break;
	case TERSE:
		if ((count & 3) == 3)
			printf("%s\n", rec->uname);
		else
			printf("%-16s ", rec->uname);
		break;
	case TERSE|ONECOL:
		printf("%s\n", rec->uname);
		break;
	}
	++count;
}
