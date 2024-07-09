
#include <sys/param.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define	_PATH_RESVPORTS	_PATH_VARRUN"resvports"

static size_t bufsiz = BUFSIZ;
static int debug;
static int daemon_mode;
static char *progname;
static in_port_t range_low = 600;
static in_port_t range_high = IPPORT_RESERVED - 1;
static time_t wait_time = 10*60;	/* default to ten minutes */

static char *read_services(void);
static void usage(void);
static void write_resvports(char *);

extern void chkprt_setservent();
extern void chkprt_endservent();
extern struct servent *chkprt_getservent();

char *services_filename = _PATH_SERVICES;
void (*setservent_ptr)() = setservent;
void (*endservent_ptr)() = endservent;
struct servent *(*getservent_ptr)() = getservent;

int
main(int argc, char *argv[])
{
	int ch;
	char *last_ports;

	if ((progname = strrchr(argv[0], '/')) != NULL)
		progname++;
	else
		progname = argv[0];

	while ((ch = getopt(argc, argv, "dDf:h:l:w:")) != EOF) {
		int value;

		switch (ch) {
		case 'd':
			daemon_mode = 1;
			break;
		case 'D':
			debug = 1;
			daemon_mode = 0;
			break;
		case 'f':
			services_filename = optarg;
			setservent_ptr = chkprt_setservent;
			endservent_ptr = chkprt_endservent;
			getservent_ptr = chkprt_getservent;
			break;
		case 'h':
			value = atoi(optarg);
			if (value < 1 || value > IPPORT_RESERVED)
				err(1, "invalid high range: %d");
			range_high = value;
			break;
		case 'l':
			value = atoi(optarg);
			if (value < 1 || value > IPPORT_RESERVED)
				err(1, "invalid low range: %d");
			range_low = value;
			break;
		case 'w':
			value = atoi(optarg);
			if (value < 0)
				err(1, "invalid wait time: %d");
			wait_time = value;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		usage();

	if (daemon_mode && !debug && daemon(0, 0) < 0)
		err(1, "Unable to become a daemon");

	openlog(progname, (!daemon_mode || debug) ? LOG_PERROR : 0, LOG_DAEMON);

	/* Main loop */
	for (last_ports = NULL; 1; sleep(wait_time)) {
		char *ports;

		ports = read_services();
		if (ports == NULL)
			continue;
		if (last_ports == NULL || strcmp(ports, last_ports) != 0) {
			write_resvports(ports);
			if (last_ports != NULL)
				free(last_ports);
			last_ports = ports;
		} else
			free(ports);

		if (!daemon_mode)
			break;
	}

	return(0);
}

static void
usage(void)
{

	fprintf(stderr, "Usage: %s [-dD] [-h high_port] [-l low_port]\n",
	    progname);
	exit(1);
}

static void
append_buf(char **portsp, char **cpp, in_port_t first_port, in_port_t last_port)
{
	char buf[10];
	size_t buflen;

	if (first_port != 0 && first_port != last_port)
		snprintf(buf, sizeof(buf), "%u-%u\n",
		    first_port, last_port);
	else
		snprintf(buf, sizeof(buf), "%u\n", first_port);
	buflen = strlen(buf);

	if ((size_t)(*portsp + bufsiz - *cpp) < buflen) {
		size_t pos = *cpp - *portsp;

		bufsiz *= 2;
		if ((*portsp = realloc(*portsp, bufsiz)) == NULL) {
			syslog(LOG_ERR, "malloc: %m");
			return;
		}
		*cpp = *portsp + pos;
	}

	strcpy(*cpp, buf);
	*cpp += buflen;
}

/*
 * Read services file and build a file containing a list of reserved
 * ports.  There is either one port, or the start and and of a port
 * range per line.
 *
 * Returns NULL if there is an error.
 */
static char *
read_services(void)
{
	struct servent *sp;
	in_port_t first_port;
	in_port_t last_port;
	char *ports, *cp;

	if ((ports = malloc(bufsiz)) == NULL) {
		syslog(LOG_ERR, "malloc: %m");
		return (ports);
	}
	cp = ports;

	first_port = 0;
	last_port = -1;

	setservent_ptr(1);

	while ((sp = getservent_ptr()) != NULL) {
		in_port_t port = ntohs(sp->s_port);

		if (port < range_low || port > range_high)
			continue;

		if (first_port == 0) {
			first_port = last_port = port;
			continue;
		}
		if (first_port != 0) {
			if (last_port == port)
				continue;
			if (last_port == port - 1) {
				last_port++;
				continue;
			}
		}

		append_buf(&ports, &cp, first_port, last_port);

		first_port = last_port = port;
	}

	if (first_port != 0)
		append_buf(&ports, &cp, first_port, last_port);

	endservent_ptr();

	return (ports);
}

/* 
 * Write the supplied buffer to the file.  Try to use locking at
 * first.  If locking is not available, prepend our pid to the file
 * name and rename it after we close it.
 */
static void
write_resvports(char *ports)
{
 	FILE *fp;
	int fd;
	static int flags = O_WRONLY|O_CREAT|O_EXLOCK|O_TRUNC;
	char path[PATH_MAX];

 Retry:
	if (flags & O_EXLOCK)
		snprintf(path, sizeof(path), "%s", _PATH_RESVPORTS);
	else
		snprintf(path, sizeof(path), "%s.%d", _PATH_RESVPORTS, getpid());

	fd = open(path, flags, 0644);
	if (fd < 0) {
		/* If locking not supported, retry w/o it */
		if (errno == EOPNOTSUPP && flags & O_EXLOCK) {
			flags &= ~O_EXLOCK;
			goto Retry;
		}
		syslog(LOG_ERR, "opening %s: %m", path);
		return;
	}

	fp = fdopen(fd, "w");

	(void)fprintf(fp, "%s", ports);

	if (fclose(fp) == EOF) {
		syslog(LOG_ERR, "closing %s: %m", path);
		(void)unlink(path);
		return;
	}

	/* If no locking, move this one into place */
	if (!(flags & O_EXLOCK) &&
	    rename(path, _PATH_RESVPORTS) < 0)
		syslog(LOG_ERR, "rename %s to %s: %m", path, _PATH_RESVPORTS);
}
