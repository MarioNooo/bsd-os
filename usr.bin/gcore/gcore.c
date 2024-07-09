/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gcore.c,v 2.3 2000/08/01 20:19:23 polk Exp
 */

#include <sys/types.h>
#include <sys/fcore.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define	COMPAT		1

void dosignals(void);
void handler(int);
void usage(void);

int
main(int argc, char **argv)
{
	char buffer[sizeof ("core.") + sizeof (pid_t) * 3];
	char *filename = NULL;
	pid_t pid;
	int error = 0;
	int status;
	int f;
	int c;

	while ((c = getopt(argc, argv, "c:s")) != -1)
		switch (c) {
		case 'c':
			filename = optarg;
			break;
#ifdef COMPAT
		case 's':
			/* XXX We always stop the process.  */
			break;
#endif
		default:
			usage();
			break;
		}

	argc -= optind;
	argv += optind;

#ifdef COMPAT
	/* XXX Skip the exec file argument.  */
	if (argc == 2) {
		--argc;
		++argv;
	}
#endif

	if (argc != 1)
		usage();

	if ((pid = atoi(*argv)) <= 0)
		err(1, "invalid process ID `%s'", *argv);

	if (filename == NULL) {
		snprintf(buffer, sizeof (buffer), "core.%d", pid);
		filename = buffer;
	}

	if ((f = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
		err(1, "%s", filename);

	/*
	 * We must attach to the process in order to extract a core file.
	 * This makes us the parent of the process, and stops it.
	 * Many permissions checks must succeed in order for this to work.
	 * We follow the old gcore in not removing any zero-length
	 * core file that results from an error.
	 */

	dosignals();

	if (ptrace(PT_ATTACH, pid, PT_ADDR_ANY, 0) == -1)
		err(1, "PID %d", pid);

	if (waitpid(pid, &status, 0) == -1)
		error = errno;

	if (error == 0 && fcore(f, pid, 0) == -1)
		error = errno;

	/* Make sure that we always detach from the process.  */
	ptrace(PT_DETACH, pid, PT_ADDR_ANY, 0);

	if (error) {
		errno = error;
		err(1, NULL);
	}

	close(f);

	return (0);
}

/*
 * Catch everything.  We don't want to exit without restoring pid's parent;
 * if we do so, the victim process dies from a SIGKILL.
 * This allows us to clean up if someone kills gcore because
 * (say) an interruptible NFS filesystem has hung.
 */
void
dosignals()
{
	sigset_t mask;
	struct sigaction sa, osa;
	int sig;

	sa.sa_handler = handler;
	sigemptyset(&mask);
	sa.sa_mask = mask;
	sa.sa_flags = 0;

	for (sig = 1; sig < NSIG; ++sig) {

		/* Don't bother with signals that are normally discarded.  */
		switch (sig) {
		case SIGURG:
		case SIGCONT:
		case SIGCHLD:
		case SIGIO:
		case SIGWINCH:
		case SIGINFO:
			continue;
		default:
			break;
		}

		/* Don't catch signals that are currently ignored.  */
		if (sigaction(sig, NULL, &osa) == -1)
			continue;
		if (osa.sa_handler != SIG_DFL)
			continue;

		sigaction(sig, &sa, NULL);
	}
}

void
handler(int unused)
{

	/* We use this handler to generate EINTR errors.  */
}

void
usage()
{

	fprintf(stderr, "usage: gcore [-c corefile] pid\n");
	exit(1);
}
