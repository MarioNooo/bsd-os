#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/license.h>
#include <utmp.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

int
check_license(who, quietlog)
	char *who;
	int quietlog;
{
	FILE *fp;
	struct utmp ut;
	int i, mib[2], limit;
	size_t size;
	char **names, *p;
	int whocnt = 0;

	mib[0] = CTL_KERN;
	mib[1] = KERN_LICENSE_FLAGS;
	size = sizeof(i);

	if (sysctl(mib, 2, &i, &size, 0, 0) == 0 &&
	    ((i & LICENSE_DEMO) || (!quietlog && (i & LICENSE_BETA))))
		printf("This system is running a%s%s copy of BSD/OS\n\n",
			(i & LICENSE_DEMO) ? " DEMO" : "",
			(i & LICENSE_BETA) ? " BETA" : "");

	mib[0] = CTL_KERN;
	mib[1] = KERN_LICENSE_USERS;
	size = sizeof(limit);

	if (sysctl(mib, 2, &limit, &size, 0, 0) < 0)
		return (0);		/* not configured */

	if (limit == LICENSE_UNLIMITED)
		return (0);		/* unlimited */

	names = malloc((sizeof(char *) + UT_NAMESIZE) * limit);

	if (names == NULL) {
		syslog(LOG_ERR, "unable to check user limits: %m");
		return (0);
	}

	memset(names, 0, (sizeof(char *) + UT_NAMESIZE) * limit);

	p = (char *)(names + limit);
	for (size = 0; size < limit; ++size) {
		names[size] = p;
		p += UT_NAMESIZE;
	}

	/*
	 * If they don't have a utmp, they should not be running!
	 */
	if ((fp = fopen(_PATH_UTMP, "r")) == NULL) {
		free(names);
		return (1);
	}

	size = 0;
	while (fread(&ut, sizeof(ut), 1, fp) == 1) {
		if (ut.ut_line[0] && ut.ut_name[0]) {
			if (strncmp(ut.ut_name, who, UT_NAMESIZE) == 0) {
				if (whocnt++) {
					who = NULL;
					size = 0;
					break;
				}
			}
			for (i = 0; i < size && i < limit; ++i)
				if (strncmp(ut.ut_name, names[i], UT_NAMESIZE) == 0)
					break;
			if (i == size || i == limit) {
				++size;
				if (i < limit)
					strncpy(names[i], ut.ut_name,
					    UT_NAMESIZE);
			}
		}
	}
	free(names);
	fclose(fp);
	return (size > limit);
}
