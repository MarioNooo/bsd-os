#include <sys/types.h>
#include <sys/queue.h>

#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <utmp.h>

/* A start and stop time, i.e. a login session. */
typedef struct _session {
	time_t	start;			/* Start time. */
	time_t	stop;			/* Stop time. */
} SESSION;

/*
 * We maintain a list of USER structures, one per user account for which we
 * have any login sessions.  Each USER structure contains an array of login
 * times.
 */
typedef struct _user {
	LIST_ENTRY(_user)q;		/* Linked list of users. */

	char	uname[UT_NAMESIZE + 1];	/* User login. */

	SESSION	*tarray;		/* Array of start/stop times. */
	size_t	 tcnt;			/* Filled elements in tarray. */
	size_t	 tlen;			/* Total slots in tarray. */

	time_t	 total;			/* Total time for this user. */
	int	 ignore;		/* If not a command-line user. */
} USER;
LIST_HEAD(_userh, _user) userq;

/*
 * We maintain a list of tty lines that we've heard of, each of which points
 * to the USER structure who is currently logged in on the line.
 */
typedef struct _tty {
	LIST_ENTRY(_tty)q;		/* Linked list of ttys. */

	time_t	 start;			/* Time of login. */
	int	 num;			/* Tty number. */
	char	 line[UT_LINESIZE + 1];	/* Tty name. */

	USER	*userp;			/* Currently associated user. */
} TTY;
LIST_HEAD(_ttyh, _tty) ttyq;

#ifdef DEBUG
#define	OPTS	"D:dpw:"
#else
#define	OPTS	"dpw:"
#endif

time_t	lasttime;			/* Last login time. */
int	debug_raw, debug_warn;		/* Debugging flags. */
char	**userlist;			/* NULL terminated list of users. */

void	process __P((char *, char *, int, time_t));
void	raw_input __P((char *, char *, time_t));
void	shutdown __P((void));
int	tcmp __P((const void *, const void *));
void	totals __P((int, int));
int	ucmp __P((const void *, const void *));
void	update __P((TTY *, char *, time_t));
void	usage __P((void));
void	ut __P((USER *, time_t, time_t));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *wfp;
	struct utmp ubuf;
	int ch, dflag, num, pflag;
	char *p, *t, *wtmpfile, line[UT_LINESIZE + 1], name[UT_NAMESIZE + 1];

	dflag = pflag = 0;
	wtmpfile = _PATH_WTMP;
	while ((ch = getopt(argc, argv, OPTS)) != EOF)
		switch (ch) {
		case 'D':
			switch (optarg[0]) {
			case 'r':
				debug_raw = 1;
				break;
			case 'w':
				debug_warn = 1;
				break;
			default:
				usage();
				/* NOTREACHED */
			}
			break;
		case 'd':
			dflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'w':
			wtmpfile = optarg;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	/* Any other arguments are individual users we're focusing on. */
	userlist = argv;

	/* Initialize user and tty queues. */
	LIST_INIT(&userq);
	LIST_INIT(&ttyq);

	/* Open the wtmp file we're using. */
	if ((wfp = fopen(wtmpfile, "r")) == NULL)
		err(1, "%s", wtmpfile);

	/*
	 * Process each record in the file.  Utmp records are space padded
	 * and not nul terminated, so we create our own copy.
	 */
	while (fread(&ubuf, sizeof(ubuf), 1, wfp) == 1) {
		for (t = name, p = ubuf.ut_name;
		    *p != '\0' && *p != ' ' && p < ubuf.ut_name + UT_NAMESIZE;)
			*t++ = *p++;
		*t = '\0';
		for (num = -1, t = line, p = ubuf.ut_line; *p != '\0'
		    && *p != ' ' && p < ubuf.ut_line + UT_LINESIZE;) {
			if (num == -1 && isdigit(*p))
				num = atoi(p);
			*t++ = *p++;
		}
		*t = '\0';
		if (debug_raw)
			raw_input(name, line, (time_t)ubuf.ut_time);
		process(name, line, num, (time_t)ubuf.ut_time);
	}
	if (ferror(wfp))
		err(1, "%s", wtmpfile);

	/* Close out open logins. */
	shutdown();

	/* Display output. */
	totals(dflag, pflag);

	exit(0);
}

/*
 * process --
 *	Handle a single utmp entry.
 */
void
process(name, line, num, tval)
	char *name, *line;
	int num;
	time_t tval;
{
	static time_t time_before;
	TTY *tp;
	long offset;

	/* Check for special records. */
	switch (line[0]) {
	case '|':				/* Date before change. */
		time_before = tval;
		return;
	case '{':				/* Date after change. */
		/*
		 * If it's a date change, adjust all current logins.  Don't
		 * adjust the saved last login time, but that's unlikely to
		 * cause problems.
		 */
		if (time_before == 0) {
			if (debug_warn)
				warnx("wtmp: corrupted, missing | record");
		} else {
			offset = tval - time_before;
			for (tp = ttyq.lh_first; tp != NULL; tp = tp->q.le_next)
				tp->start += offset;
			time_before = 0;
		}
		return;
	case '~':				/* Reboot/shutdown. */
		/*
		 * If the system rebooted, everyone logged out, total the time
		 * and reset all of the lines.
		 */
		for (tp = ttyq.lh_first; tp != NULL; tp = tp->q.le_next)
			if (tp->userp != NULL)
				update(tp, NULL, tval);
		return;
	}

	/* Find this tty's information, or create a new tty entry. */
	for (tp = ttyq.lh_first; tp != NULL; tp = tp->q.le_next)
		if (num == tp->num && !strcmp(tp->line, line))
			break;
	if (tp == NULL) {
		if ((tp = calloc(1, sizeof(TTY))) == NULL)
			err(1, NULL);
		LIST_INSERT_HEAD(&ttyq, tp, q);
		tp->num = num;
		(void)strcpy(tp->line, line);
	}
	update(tp, name, tval);
}

/*
 * shutdown --
 *	Close down any open login records.
 */
void
shutdown()
{
	struct tm lt;
	TTY *tp;
	time_t now, tval;

	/*
	 * Calculate midnight of the last day for which we saw a login
	 * record.
	 */
	lt = *localtime(&lasttime);
	lt.tm_sec = 0;
	lt.tm_min = 0;
	lt.tm_hour = 24;
	if ((tval = mktime(&lt)) == -1)
		err(1, "mktime");

	/*
	 * If that time is later than "right now", then don't try and
	 * predict the future.
	 */
	time(&now);
	if (now < tval)
		tval = now;

	/*
	 * For any lines that are currently logged in, pretend they logged
	 * out at that time.
	 */
	for (tp = ttyq.lh_first; tp != NULL; tp = tp->q.le_next)
		if (tp->userp != NULL)
			update(tp, NULL, tval);
}

/*
 * update --
 *	Update a tty line and associated user entry.
 */
void
update(tp, name, tval)
	TTY *tp;
	char *name;
	time_t tval;
{
	USER *up;

	/*
	 * If we already have a user record associated with this tty, then
	 * it's a logout, and we update it with the accumulated time.
	 */
	if ((up = tp->userp) != NULL) {
		if (tp->start > tval) {
			if (debug_warn)
				warnx("wtmp: corrupted, records out of order");
		} else {
			if (up->tcnt == up->tlen) {
				up->tlen += 20;
				if ((up->tarray = realloc(up->tarray,
				    up->tlen * sizeof(up->tarray[0]))) == NULL)
					err(1, NULL);
			}
			up->tarray[up->tcnt].start = tp->start;
			up->tarray[up->tcnt].stop = tval;
			++up->tcnt;
		}
		tp->userp = NULL;
		return;
	}

	/* If no user information with the login record, we're confused... */
	if (name[0] == '\0') {
		if (debug_warn)
			warnx("wtmp: corrupted, unexpectedly empty name field");
		return;
	}

	/*
	 * Set the start time and find the user's information, or create a
	 * new user entry.
	 */
	for (up = userq.lh_first; up != NULL; up = up->q.le_next)
		if (up->uname[0] == name[0] &&
		    !strncmp(up->uname, name, UT_NAMESIZE))
			break;
	if (up == NULL) {
		if ((up = calloc(1, sizeof(USER))) == NULL)
			err(1, NULL);
		LIST_INSERT_HEAD(&userq, up, q);
		(void)strcpy(up->uname, name);
	}
	tp->userp = up;
	tp->start = tval;

	/*
	 * Save the last known login time.  We always trust the last one
	 * that we've seen, on the grounds that the longer the system is
	 * running, the more likely the time is to be correct.
	 */
	lasttime = tval;
}

/*
 * totals --
 *	Display the totals.
 */
void
totals(perday, individual)
	int perday, individual;
{
	struct tm lt;
	USER **ap, **tup, *up;
	u_quad_t total;
	time_t min, max;
	int cnt;
	char **p;

	/* Figure out which users we care about. */
	for (cnt = 0, up = userq.lh_first; up != NULL; up = up->q.le_next) {
		++cnt;
		up->ignore = 0;
		if (*userlist == NULL)
			continue;
		for (p = userlist;; ++p) {
			if (*p == NULL) {
				--cnt;
				up->ignore = 1;
				break;
			}
			if (!strcmp(up->uname, *p))
				break;
		}
	}

	/* If no users we care about, quit. */
	if (cnt == 0)
		return;

	/* Build a sorted list of users. */
	if ((ap = tup = calloc(cnt + 1, sizeof(USER *))) == NULL)
		err(1, NULL);
	for (up = userq.lh_first; up != NULL; up = up->q.le_next)
		if (!up->ignore)
			*tup++ = up;
	*tup = NULL;
	qsort(ap, cnt, sizeof(USER *), ucmp);

	if (perday) {
		/*
		 * If we're doing per-day totals, sort each user's entries,
		 * and find the earliest start and latest stop time.
		 */
		min = LONG_MAX;
		max = 0;
		for (tup = ap; (up = *tup) != NULL; ++tup) {
			if (up->ignore)
				continue;
			qsort(up->tarray,
			    up->tcnt, sizeof(up->tarray[0]), tcmp);
			if (up->tarray[0].start < min)
				min = up->tarray[0].start;
			if (up->tarray[up->tcnt - 1].stop > max)
				max = up->tarray[up->tcnt - 1].stop;
		}

		/*
		 * Adjust min to be midnight of the first day for which we
		 * found a time, and max to be midnight of the last day.
		 */
		lt = *localtime(&min);
		lt.tm_sec = 0;
		lt.tm_min = 0;
		lt.tm_hour = 0;
		if ((min = mktime(&lt)) == -1)
			err(1, "mktime");
		lt = *localtime(&max);
		lt.tm_sec = 0;
		lt.tm_min = 0;
		lt.tm_hour = 24;
		if ((max = mktime(&lt)) == -1)
			err(1, "mktime");

		/* Walk through the days, displaying totals. */
		for (; min < max; min += SECSPERDAY) {
			for (total = 0, tup = ap; (up = *tup) != NULL; ++tup) {
				ut(up, min, min + SECSPERDAY);
				total += up->total;
			}
			if (total == 0)
				continue;
			if (individual)
				for (tup = ap; (up = *tup) != NULL; ++tup) {
					if (up->total == 0)
						continue;
					(void)printf("\t%-*.*s %6.2f\n",
					    UT_NAMESIZE, UT_NAMESIZE, up->uname,
					    up->total / (double)SECSPERHOUR);
				}
			(void)printf("%.6s\ttotal %9.2f\n",
			    ctime(&min) + 4, total / (double)SECSPERHOUR);
		}
	} else {
		/* Display the information. */
		for (total = 0, tup = ap; (up = *tup) != NULL; ++tup) {
			if (up->ignore)
				continue;
			ut(up, 0, (time_t)LONG_MAX);
			total += up->total;
			if (individual)
				(void)printf("\t%-*.*s %6.2f\n",
				    UT_NAMESIZE, UT_NAMESIZE,
				    up->uname, up->total / (double)SECSPERHOUR);
		}
		(void)printf("\ttotal %9.2f\n", total / (double)SECSPERHOUR);
	}
}

/*
 * ut --
 *	Calculate the total time between min and max for a user.
 */
void
ut(up, min, max)
	USER *up;
	time_t min, max;
{
	SESSION *sp;
	int i;

	up->total = 0;
	for (sp = up->tarray, i = up->tcnt; i--; ++sp) {
		if (sp->stop <= min)
			continue;
		if (sp->start >= max)
			break;
		up->total +=
		    (sp->stop < max ? sp->stop : max) -
		    (sp->start > min ? sp->start : min);
	}
}

/*
 * tcmp --
 *	Comparison routine to sort login times.
 */
int
tcmp(a, b)
	const void *a, *b;
{
	return (((SESSION *)a)->start - ((SESSION *)b)->start);
}

/*
 * ucmp --
 *	Comparison routine to sort user structures.
 */
int
ucmp(a, b)
	const void *a, *b;
{
	return (strcmp((*(USER **)a)->uname, (*(USER **)b)->uname));
}

/*
 * raw_input --
 *	Debugging, display the raw records.
 */
void
raw_input(name, line, tval)
	char *name, *line;
	time_t tval;
{
	(void)printf("%24.24s: line <%s> name <%s> time (%lu)\n",
	    ctime(&tval), line, name, (u_long)tval);
}

void
usage()
{
#ifdef DEBUG
	(void)fprintf(stderr,
	    "usage: ac [-dp] [-D r | w] [-w file] [users ...]\n");
#else
	(void)fprintf(stderr, "usage: ac [-dp] [-w file] [users ...]\n");
#endif
	exit(1);
}
