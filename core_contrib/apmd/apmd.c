#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockkern.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <machine/apm.h>
#include <machine/apmioctl.h>

#include <err.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#define	LIBDIR	"/usr/libdata/apmd"

struct events {
	int num;
	char *name;
	struct timespec last;
} events[] = {
	{APM_EVENT_STANDBY_REQ, "standby_req"},
	{APM_EVENT_SUSPEND_REQ, "suspend_req"},
	{APM_EVENT_NORMAL_RESUME, "normal_resume"},
	{APM_EVENT_CRITICAL_RESUME, "critical_resume"},
	{APM_EVENT_LOW_BATTERY, "low_battery"},
	{APM_EVENT_STATUS_CHANGE, "status_change"},
	{APM_EVENT_UPDATE_TIME, "update_time"},
	{APM_EVENT_CRITICAL_SUSPEND, "critical_suspend"},
	{APM_EVENT_USER_STANDBY_REQ, "user_standby_req"},
	{APM_EVENT_USER_SUSPEND_REQ, "user_suspend_req"},
	{APM_EVENT_STANDBY_RESUME, "standby_resume"},
	{APM_EVENT_BATTERY, "battery"},
	{APM_EVENT_BATTERY_0, "battery_0"},
	{APM_EVENT_BATTERY_1, "battery_1"},
	{APM_EVENT_BATTERY_2, "battery_2"},
	{APM_EVENT_BATTERY_3, "battery_3"},
	{APM_EVENT_ERROR_RESUME, "error_resume"},
	{APM_EVENT_CLOCK_CHANGE, "clock_change"},
	{-1, NULL}
};
long	threshhold = 2*1000;
int	debug = 0;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern void reaper(int), runscript(char *[]);
	int logfd;
	struct sockaddr sa;
	struct apmlog a;
	char ibuf[12], ebuf[9];
	char *execargs[4];
	char *libdir;
	char *type;
	int r;
	int i;
	long diff;
	int c;
	int wdo;
	int mode;

	libdir = LIBDIR;
	mode = -1;
	while ((c = getopt(argc, argv, "d:t:m:x")) != EOF) {
		switch(c) {
		case 'd':
			libdir = optarg;
			break;

		case 't':
			threshhold = atoi(optarg);
			break;

		case 'm':
			if (strcmp(optarg, "user") == 0)
				mode = 1;
			else if (strcmp(optarg, "auto") == 0)
				mode = 0;
			else
				goto usage;
			break;

		case 'x':
			debug++; break;

		default: usage:
			fprintf(stderr, "Usage: apmd [-x] [-d script-dir]"
					" [-m {user|auto}] [-t thresh]\n");
			return (1);
		}
	}

	if (chdir(libdir) < 0)
		err(2, "%s", libdir);

	if (mode >= 0) {
		if ((logfd = open("/dev/apm", O_RDWR)) < 0)
			err(1, "/dev/apm");
		if (ioctl(logfd, PIOCGAPMMODE, &i) < 0)
			err(1, "PIOCGAPMMODE");
		if (i != mode && ioctl(logfd, PIOCSAPMMODE, &mode) < 0)
			err(1, "PIOCSAPMMODE");
		close(logfd);
	}

	signal(SIGCHLD, reaper);

	if ((logfd = socket(PF_KERNEL, SOCK_RAW, APM_PROTO)) < 0)
		err(1, "APM Logging Socket");

	memset(&sa, 0, sizeof(sa));
	sa.sa_len = 2;
	sa.sa_family = AF_KERNEL;

	if (bind(logfd, &sa, 2))
		err(1, "APM Logging Socket");

	if (!debug && daemon(1, 0) < 0)
		warn("daemon");

	while ((r = read(logfd, (char *)&a, sizeof(a))) >= 0) {
		if (r <= 0 && errno == EINTR)
			continue;
		if (r < sizeof(a) || r < a.length) {
			err(1, "only read %d of %d bytes\n", r,
			    r < sizeof(a) ? sizeof(a) : a.length);
		}
		for (i = 0; events[i].name; i++)
			if (a.event == events[i].num)
				break;
		sprintf(ibuf, "%u", a.info);
		if ((type = events[i].name) == NULL) {
			sprintf(ebuf, "%08x", a.event);
			wdo = 1;
			execargs[0] = "other";
			execargs[1] = ebuf;
			execargs[2] = ibuf;
			execargs[3] = 0;
		} else {
			diff = a.when.tv_nsec - events[i].last.tv_nsec;
			if (diff < 0) {
				diff += 1000000000;
				events[i].last.tv_sec++;
			}
			diff = (diff+500000)/1000000 + (a.when.tv_sec - events[i].last.tv_sec)*1000;
			wdo = (events[i].last.tv_sec == 0) || (diff >= threshhold);
			events[i].last = a.when;
			execargs[0] = type;
			execargs[1] = ibuf;
			execargs[2] = 0;
		}
		if (debug)
			printf("%.15s.%06ld %s %s%s\n", ctime(&a.when.tv_sec)+4,
			    a.when.tv_nsec/1000, type, ibuf, wdo ? "" : " ignored");
		if (wdo && access(type, X_OK) == 0)
			runscript(execargs);
	}
	return (0);
}

void
runscript(char *rargs[])
{
	int pid;

	switch(pid = fork()) {
	case 0:
		execv(rargs[0], rargs);
		exit(-1);
		/*NOTREACHED*/

	case -1:
		warn(NULL);
		break;

	default:
		if (debug)
			printf("Forking %d\n", pid);
		break;
	}
}

void
reaper(int sig)
{
	int status, pid;

	while ((pid = wait3(&status, WNOHANG, NULL)) > 0)
		if (debug)
			printf("Reaped %d status %x\n", pid, status);
}
