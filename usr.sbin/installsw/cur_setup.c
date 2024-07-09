/*-
 * Copyright (c) 1993, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI cur_setup.c,v 2.7 1996/09/16 16:50:11 prb Exp
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/queue.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "installsw.h"
#include "pathnames.h"

static int	 chk_host __P((void));
static int	 chk_device __P((void));
static void	 get_device __P((void));
static char	*query __P((char *, char *, char **, int));

#define	MOUNT_MSG \
"The filesystems on which you will be installing must be mounted\n\
before running this program.  If you are installing from CDROM or\n\
floppy, you must also have the CDROM or floppy mounted.  If you\n\
are installing from a remote system, the network must already be\n\
configured, and you should have remote access permission from the\n\
machine that hosts the installation media.\n"

/*
 * setup --
 *	Get setup information.
 */
void
setup()
{
	char *ans;

	/* If not an expert, describe what we're doing. */
	if (!express && !expert) {
		(void)printf("%s", MOUNT_MSG);
		if (!yesno("Continue?"))
			exit(0);
	}

	/* Get a media choice. */
	if (media == M_NONE) {
		char *media_answers[] = {"cdrom", "floppy", "tape", NULL};

		ans = query("Are you installing from cdrom, floppy or tape?",
		    DEFAULT_MEDIA, media_answers, 0);
		if (!strcmp(ans, "cdrom"))
			media = M_CDROM;
		else if (!strcmp(ans, "floppy"))
			media = M_FLOPPY;
		else
			media = M_TAPE;
	}

	/* Get the machine (location or remote), and host name. */
	if (location == L_NONE) {
		char *location_answers[] = {"local", "remote", NULL};

		ans = query("Is the distribution package local or remote?",
		    "local", location_answers, 0);
		if (!strcmp(ans, "local"))
			location = L_LOCAL;
		else
			location = L_REMOTE;
	}
	if (location == L_REMOTE)
		for (;;) {
			if (remote_host == NULL)
				remote_host = query("Hostname of remote host?",
				    NULL, NULL, 1);
			if (!chk_host())
				break;

			/* If command-line selection, it's fatal. */
			if (express)
				exit (1);

			remote_host = NULL;	/* XXX: Memory leak. */
		}
	
	/* Get the path of the package area. */
	for (;;) {
		if (!devset)
			get_device();
		if (!chk_device())
			break;

		/* If command-line selection, it's fatal. */
		if (express)
			exit (1);

		devset = 0;
	}
}

/*
 * get_device --
 *	Get a device name from the user.
 */
static void
get_device()
{
	char question[1024];

	switch (media) {
	case M_CDROM:
		switch (location) {
		case L_LOCAL:
			device =
			    query("On what directory is the CDROM mounted?",
			    CD_MOUNTPT, NULL, 1);
			break;
		case L_REMOTE:
			(void)snprintf(question, sizeof(question),
			    "On what directory on %s is the CDROM mounted?",
			    remote_host);
			device = query(question, CD_MOUNTPT, NULL, 1);
			break;
		default:
			abort();
		}
		break;
	case M_FLOPPY:
		switch (location) {
		case L_LOCAL:
			device =
			    query("On what directory is the floppy mounted?",
			    FL_MOUNTPT, NULL, 1);
			break;
		case L_REMOTE:
			(void)snprintf(question, sizeof(question),
			    "On what directory on %s is the floppy mounted?",
			    remote_host);
			device = query(question, FL_MOUNTPT, NULL, 1);
			break;
		default:
			abort();
		}
		break;
	case M_TAPE:
		switch (location) {
		case L_LOCAL:
			device =
			query("What is the name of the no-rewind tape device?",
			        TAPE_DEVICE, NULL, 1);
			break;
		case L_REMOTE:
			(void)snprintf(question, sizeof(question),
		"What is the name of the no-rewind tape device on %s?",
			    remote_host);
			device =
			    query(question, TAPE_DEVICE, NULL, 1);
			break;
		default:
			abort();
		}
		break;
	default:	
		abort();
	}
	devset = 1;
}

/*
 * chk_device --
 *	Check the validity of the device.
 */
static int
chk_device()
{
	struct stat sb;

	switch (location) {
	case L_LOCAL:
		if (stat(device, &sb)) {
			warn("%s", device);
			return (1);
		}
		switch (media) {
		case M_CDROM:
		case M_FLOPPY:
			if (!S_ISDIR(sb.st_mode)) {
				warnx("%s: is not a possible mount point",
				    device);
				return (1);
			}
			break;
		case M_TAPE:
			if (!S_ISCHR(sb.st_mode)) {
				warnx("%s: is not a tape drive", device);
				return (1);
			}
			break;
		default:
			abort();
		}
		break;
	case L_REMOTE:
		switch (media) {
		case M_FLOPPY:
			break;
		case M_CDROM:
			if (determine_name_mapping())
				return (1);
			break;
		case M_TAPE:
			/* Try and rewind the tape... */
			if (tape_setup(0, 0)) {
				warnx("%s: tape access failed", device);
				return (1);
			}
			break;
		default:	
			abort();
		}
		break;
	default:	
		abort();
	}
	return (0);
}

/*
 * chk_host --
 *	Execute a simple command (echo) on the remote host.
 */
static int
chk_host()
{
	char buf[1024];

	(void)printf("\n\tTrying to contact %s...\n", remote_host);

	(void)snprintf(buf, sizeof(buf), "> %s", _PATH_DEVNULL);

	/* Don't use _PATH_ECHO, use the most portable path. */
	if (remote_local_command("/bin/echo", buf, 0)) {
		(void)printf("\tfailed!\n");
		return (1);
	}
	(void)printf("\tsucceeded.\n");
	return (0);
}

/*
 * yesno --
 *	Ask a yes/no question, return 1 if yes, 0 if no.
 */
int
yesno(prompt)
	char *prompt;
{
	char *ans, *yes_no[] = {"yes", "no", NULL};

	ans = query(prompt, "yes", yes_no, 0);
	return (ans[0] == 'y');
}

/*
 * query --
 *	Prompt the user for data using the given prompt string.  Accept
 *	the default if specified.  Otherwise, require that the response
 *	either fully match or be a unique prefix of one of the possible
 *	choices.
 */
static char *
query(prompt, deflt, choices, copy)
	char *prompt, *deflt, **choices;
	int copy;
{
	static char answer_buf[256];
	size_t len;
	int pmatch;
	char *p, **cp;

	for (;;) {
		if (deflt == NULL)
			(void)printf("\n%s: ", prompt);
		else
			(void)printf("\n%s [%s]: ", prompt, deflt);
		(void)fflush(stdout);

		if (fgets(answer_buf, sizeof(answer_buf), stdin) == NULL) {
			if (feof(stdin))
				exit (0);
			err(1, "stdin");
		}
		if ((p = strchr(answer_buf, '\n')) == NULL)
			errx(1, "stdin: input overflow");
		*p = '\0';

		if (answer_buf[0] == '\0') {
			if (deflt != NULL) {
				(void)strcpy(answer_buf, deflt);
				break;
			}
			continue;
		}
		if (choices == NULL)
			break;
		/* Check for a matching choice. */
		len = strlen(answer_buf);
		for (pmatch = 0,
		    cp = choices; *cp != NULL && **cp != '\0'; ++cp) {
			if (!strcasecmp(answer_buf, *cp))
				goto found;
			if (strlen(*cp) > len &&
			    !strncasecmp(answer_buf, *cp, len)) {
				++pmatch;
				p = *cp;
			}
		}
		if (pmatch == 1) {
			(void)strcpy(answer_buf, p);
found:			for (p = answer_buf; *p != '\0'; ++p)
				if (isupper(*p))
					*p = tolower(*p);
			break;
		}
		(void)printf("Possible responses are: ");
		for (cp = choices; *cp != NULL && **cp != '\0'; ++cp)
			(void)printf("%s ", *cp);
	}

	if (copy) {
		if ((p = strdup(answer_buf)) == NULL)
			err(1, NULL);
		return (p);
	}
	return (answer_buf);
}
