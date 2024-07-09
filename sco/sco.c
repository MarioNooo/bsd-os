/*	BSDI sco.c,v 2.1 1995/02/03 15:12:50 polk Exp	*/

#include <sys/param.h>
#include <machine/segments.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char hostname[MAXHOSTNAMELEN+1];

int
main(argc, argv)
	int argc;
	char **argv;
{
	union descriptor d;
	char *s, *p, *path;
	int len;

	/*
	 * Merely calling setdescriptor() makes us an 'emulated' program,
	 * in spite of the fact that we weren't loaded with the emulator.
	 */
	if (getdescriptor(LSEL(LDEFCALLS_SEL, SEL_UPL), &d) == -1)
		err(1, "getdescriptor");
	if (setdescriptor(LSEL(LDEFCALLS_SEL, SEL_UPL), &d) == -1)
		err(1, "setdescriptor");

	/*
	 * A hack specifically to allow us to replace 'echo' in shell scripts.
	 * If $PATH doesn't contain %builtin, we append it, to force
	 * ash to search for builtins after regular commands.
	 */
	if ((p = getenv("PATH")) && strstr(p, ":%builtin") == 0) {
		if ((path = malloc(strlen(p) + sizeof ":%builtin")) == 0)
			warn("path malloc");
		else {
			strcpy(path, p);
			strcat(path, ":%builtin");
			setenv("PATH", path, 1);
		}
	}

	/*
	 * Another hack: force root to put /etc in its path.
	 * /etc on System V contains executable files...
	 */
	if (geteuid() == 0 && (p = getenv("PATH")) &&
	    strncmp(p, "/etc:", 5) != 0 && strstr(p, ":/etc") == 0) {
		if ((path = malloc(strlen(p) + sizeof "/etc:")) == 0)
			warn("path malloc");
		else {
			strcpy(path, "/etc:");
			strcat(path, p);
			setenv("PATH", path, 1);
		}
	}

	/*
	 * And another hack: if DISPLAY is ":0.*", prepend the hostname.
	 */
	if ((p = getenv("DISPLAY")) && *p == ':') {
		if (gethostname(hostname, sizeof hostname) == -1)
			warn("gethostname");
		else if ((s = malloc(strlen(hostname) + strlen(p) + 1)) == 0)
			warn("display malloc");
		else {
			strcpy(s, hostname);
			strcat(s, p);
			setenv("DISPLAY", s, 1);
		}
	}

	execvp(argv[1], argv + 1);
	err(1, "execvp");
}
