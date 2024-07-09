/*	BSDI tokeninit.c,v 1.1 1996/08/26 20:27:28 prb Exp
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
#include <sys/resource.h>

#include "token.h"
#include "tokendb.h"

static	void	strip_crlf(char *);
static	void	prompt_for_secret(int, char*);
static	int	parse_secret(int, char *, unsigned char *);

main(int argc, char **argv)
{
	unsigned cmd = TOKEN_INITUSER;
	int	c;
	int	errors = 0;
	int	verbose = 0;
	int	hexformat = 0;
	int	modes = 0;
	char	seed[80];
	unsigned char	secret[9];
	char	*optstr;

	struct rlimit cds;

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog(NULL, LOG_ODELAY, LOG_AUTH);

	cds.rlim_cur = 0;
	cds.rlim_max = 0;
	if (setrlimit(RLIMIT_CORE, &cds) < 0)
		syslog(LOG_ERR, "couldn't set core dump size to 0: %m");

	if (token_init(argv[0]) < 0) {
		syslog(LOG_ERR, "unknown token type");
		errx(1, "unknown token type");
	}

	if (tt->options & TOKEN_HEXINIT)
		optstr = "fhm:sv";
	else
		optstr = "fm:sv";

    	while ((c = getopt(argc, argv, optstr)) != EOF)
		switch (c) {
		case 'f':	/* force initialize existing user account */
			cmd |= TOKEN_FORCEINIT;
			break;

		case 'h':
			hexformat++;
			break;

		case 'm':
			if (c = token_mode(optarg))
				modes |= c;
			else
				errx(1, "%s: unknown mode");
			break;

		case 's':	/* generate seed during initialization */
			cmd |= TOKEN_GENSECRET;
			break;

		case 'v':	/* verbose */
			verbose++;
			break;
		default:
			fprintf(stderr,
			   "Usage: %sinit [-f%ssv] username [ username ... ]\n",
			    tt->name, (tt->options & TOKEN_HEXINIT) ? "h" : "");
			exit(1);
		}

	if ((modes & ~TOKEN_RIM) == 0)
		modes |= tt->defmode;

	argc -= optind;
	argv = &argv[optind];

	while (argc--) {
		if (verbose) {
			printf("Adding %s to %s database\n", *argv, tt->proper);
			fflush(stdout);
		}
		if (!(cmd & TOKEN_GENSECRET)) {
			prompt_for_secret(hexformat, *argv);

			if (fgets(seed, sizeof(seed), stdin) == NULL) {
				fprintf(stderr,
			    	    "%sinit: No seed supplied for token.\n",
				    tt->name);
				exit(1);
			}
			strip_crlf(seed);
			if (strlen(seed) == 0) {
				fprintf(stderr,
				    "%sinit: No seed supplied for token.\n",
				    tt->name);
				exit(1);
			}
			memset(secret, 0, sizeof(secret));
			if (parse_secret(hexformat, seed, secret)) {
				fprintf(stderr,
				    "%sinit: Invalid secret entered.\n",
				    tt->name);
				exit(1);
			}
		}
		switch (tokenuserinit(cmd, *argv, secret, modes)) {
		case 0:
			syslog(LOG_INFO, "User %s initialized in %s database",
			    *argv, tt->proper);
			break;
		case 1:
			warnx("%s already exists in %s database!\n",
			    *argv, tt->proper);
			syslog(LOG_INFO, "%s already exists in %s database",
			    *argv, tt->proper);
			errors++;
			break;
		case -1:
			warnx("Error initializing user %s in %s database.\n",
			    *argv, tt->proper);
			syslog(LOG_INFO,
			    "Error initializing user %s in %s database: %m",
			    *argv, tt->proper);
			errors++;
		}
		*argv++;
	}
	exit(errors);
}

/*
 * Strip trailing cr/lf from a line of text
 */

void
strip_crlf(char *buf)
{
        char *cp;

        if((cp = strchr(buf,'\r')) != NULL)
                *cp = '\0';

        if((cp = strchr(buf,'\n')) != NULL)
                *cp = '\0';
}

/*
 * Parse the 8 octal numbers or a 16 digit hex string into a token secret
 */

static	int
parse_secret(int hexformat, char *seed, unsigned char *secret)
{
	int i;
	unsigned tmp[8];

	if (hexformat) {
		if ((i = sscanf(seed, "%02x %02x %02x %02x %02x %02x %02x %02x",
		    &tmp[0], &tmp[1], &tmp[2], &tmp[3],
		    &tmp[4], &tmp[5], &tmp[6], &tmp[7])) != 8)
			return (-1);
	} else {
		if ((i = sscanf(seed, "%o %o %o %o %o %o %o %o",
		    &tmp[0], &tmp[1], &tmp[2], &tmp[3],
		    &tmp[4], &tmp[5], &tmp[6], &tmp[7])) != 8)
			return (-1);
	}
	for (i=0; i < 8; i++)
		secret[i] = tmp[i] & 0xff;

	return (0);
}

/*
 * Prompt user for seed for token
 */

static	void
prompt_for_secret(int hexformat, char* username)
{
	if (hexformat)
		printf("Enter a 16 digit hexidecimal number "
			"as a seed for %s\'s token:\n", username);
	else
		printf("Enter a series of 8 3-digit octal numbers "
			"as a seed for %s\'s token:\n", username);
}