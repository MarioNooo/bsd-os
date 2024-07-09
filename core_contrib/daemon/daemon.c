/* BSDI daemon.c,v 1.2 1996/08/15 01:11:09 jch Exp */

#include <sys/types.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, nochdir, noclose;

	nochdir = noclose = 1;
	while ((ch = getopt(argc, argv, "-cf")) != -1)
		switch (ch) {
		case '-':		/* Undocumented: backward compatible. */
			nochdir = noclose = 0;
			break;
		case 'c':
			nochdir = 0;
			break;
		case 'f':
			noclose = 0;
			break;
		case '?':
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	if (daemon(nochdir, noclose) == -1)
		err(1, NULL);

	execvp(argv[0], argv);

	/* The child is now running, so the exit status doesn't matter. */
	err(1, "%s", argv[0]);
}

void
usage()
{
	(void)fprintf(stderr, "usage: daemon [-cf] command arguments ...\n");
	exit(1);
}
