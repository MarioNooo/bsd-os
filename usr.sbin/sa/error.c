/* BSDI error.c,v 2.2 1996/02/29 18:14:50 bostic Exp */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "sa.h"

void
done(int status)
{
#ifdef DEBUG
	sigset_t all_signals;

	sigemptyset(&all_signals);
	unblock_signals(&all_signals);
#endif
	if (user_temp_name[0] != 0)
		(void) unlink(user_temp_name);
	if (command_temp_name[0] != 0)
		(void) unlink(command_temp_name);
#ifdef DEBUG
	abort();
#else
	exit(status);
#endif
}

void
terminate(void)
{
	done(1);
}

void
error(const char *irritant, const char *message)
{
	if (message) {
		if (irritant)
			fprintf(stderr, "sa: %s: %s\n", irritant, message);
		else
			fprintf(stderr, "sa: %s\n", message);
	} else {
		fprintf(stderr, "sa: ");
		perror(irritant);
	}
}

void
fatal_error(const char *irritant, const char *message, const int value)
{
	error(irritant, message);
	done(value);
}
