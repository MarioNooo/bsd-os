#include <sys/utsname.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

int
main(int argc, char **argv)
{
	char **xargv;
	char *p;
	int i;

#if 0
	fprintf(stderr, "sco-sh: ");
	fprintf(stderr, "%s", argv[0]);
	for (i = 1; i < argc; ++i)
		fprintf(stderr, " %s", argv[i]);
	fprintf(stderr, "\n");
	fflush(stderr);
#endif

	if (argc == 3 && strcmp(argv[1], "-c") == 0 &&
	    strncmp(argv[2], "/tmp/lotusid", 12) == 0) {

#define	KERNELID	"314"
#define	BUSTYPE		"ISA"
#define	SERIAL		"COM"
#define	USERS		"255"
#define	OEM		255
#define	ORIGIN		1
#define	NUMCPU		1

		struct utsname u;

		if (uname(&u) == -1)
			return -1;

		printf("\n");
		printf("System = %.*s\n", sizeof u.sysname, u.sysname);
		if (p = memchr(u.nodename, '.', sizeof u.nodename))
			*p = '\0';
		printf("Node = %.*s\n", sizeof u.nodename, u.nodename);
		printf("Release = %.*s\n", sizeof u.release, u.release);
		printf("KernelID = %s\n", KERNELID);
		printf("Machine = %.*s\n", sizeof u.machine, u.machine);
		printf("BusType = %s\n", BUSTYPE);
		printf("Serial = %s\n", SERIAL);
		printf("Users = %s\n", USERS);
		printf("OEM# = %d\n", OEM);
		printf("Origin = %d\n", ORIGIN);
		printf("NumCPU = %d\n", NUMCPU);
		printf("\n");
		fflush(stdout);

		return 0;
	}

	execve("/bin/sh", argv, environ);

	return -1;
}
