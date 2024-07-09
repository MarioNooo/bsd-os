/*
**	Copyright 1992,1995 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID prog.c,v 2.1 1996/06/11 19:27:16 vixie Exp
**
**	prog.c,v
**	Revision 2.1  1996/06/11 19:27:16  vixie
**	2.2
**	
 */

#define	TCP_IP
#define SYSEXITS

#include "condevs.h"

#ifdef	PROG

/*
**	progcls -- close program connection
*/

CallType
progcls(int fd) {
	Debug((4, "PROG CLOSE called"));

	if (fd == SYSERROR)
		return;

	(void)close(fd);

	Debug((4, "closed fd %d", fd));

	/* It is tempting to do a wait3() here, but to make sure we did not
	 * pick up some other thread's child we would have to keep track of
	 * the fd->pid mappings and send a kill() to our own child here.
	 * Since the dialer library is supposed to be reentrant, we can't
	 * do that easily.  (XXX?)
	 */
	return (0);
}

/*
**	progopn -- make a program connection
**
**	return codes:
**		>0 - file number - ok
**		FAIL - failed
*/

CallType
progopn(char *flds[]) {
	static const char	terr[] = "%s() failed: errno %d";

	const char *program_name = flds[F_CLASS];
	char *program_args = flds[F_PHONE];
	char **cpp, *cp, *args[MAXFIELDS];
	int s[2], pid, f;

	Debug((4, "progopn prog %s, args (%s)", program_name, program_args));
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, s) < 0) {
		Log("CANNOT GET SOCKET PAIR");
		return (CF_SYSTEM);
	}
	switch (pid = vfork()) {
	case (-1):
		Log("CANNOT VFORK PROCESS");
		return (CF_SYSTEM);
	case (0):
		/* Child. */
		for (f = getdtablesize() - 1;  f >= 0;  f--)
			if (f != s[0] && f != STDERR_FILENO)
				(void)close(f);
		if (s[0] != STDIN_FILENO) {
			(void)dup2(s[0], STDIN_FILENO);
			(void)close(s[0]);
		}
		(void)dup2(STDIN_FILENO, STDOUT_FILENO);
		cpp = args;
		*cpp = program_args;
		for (cp = program_args; *cp; cp++)
			if (*cp == '!') {
				*cp = '\0';
				*++cpp = cp + 1;
			}
		*++cpp = NULL;
		execve(program_name, args, NULL);
		(void)strcpy(FailMsg, "CANNOT EXECVE");
		Log("%s %s", FailMsg, program_name);
		exit(EX_SOFTWARE);
		/*NOTREACHED*/
	default:
		/* Parent. */
		break;
	}

	/* Parent. */
	(void)close(s[0]);	/* child's endpoint. */
	CU_end = progcls;
	return (s[1]);
}
#endif	/* PROG */
