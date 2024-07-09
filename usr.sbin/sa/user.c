/* BSDI user.c,v 2.2 1996/02/29 18:14:59 bostic Exp */

#include <pwd.h>
#include <stdio.h>
#include <string.h>

#include "sa.h"

int
dump_user_command(const struct acct * record)
{
	static long last_user_id = -1;
	static char user[40];

	if (record->ac_uid != last_user_id) {
		struct passwd *pw = getpwuid(record->ac_uid);

		if (pw) {
			strncpy(user, pw->pw_name, sizeof(user));
			user[sizeof(user) - 1] = 0;
		} else
			sprintf(user, "%d", record->ac_uid);
	}
	printf("%-10.10s %s\n", record->ac_comm, user);
	return TRUE;
}
