/* BSDI signal.c,v 2.2 1996/02/29 18:14:56 bostic Exp */

#include <signal.h>

#include "sa.h"

void
trap_signals(int *signals, void (*func) (void))
{
	int     i;

	for (i = 0; signals[i] != 0; ++i) {
		struct sigaction new_action;
		struct sigaction old_action;

		new_action.sa_handler = SIG_IGN;
		sigfillset(&new_action.sa_mask);
		new_action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
		(void) sigaction(signals[i], &new_action, &old_action);
		if (old_action.sa_handler != SIG_IGN) {
			new_action.sa_handler = terminate;
			(void) sigaction(signals[i], &new_action, NULL);
		}
	}
}
