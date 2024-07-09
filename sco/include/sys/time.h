/*	BSDI time.h,v 1.1 1995/07/10 18:26:25 donn Exp	*/

#ifndef	_SCO_SYS_TIME_H_
#define	_SCO_SYS_TIME_H_

#include <machine/ansi.h>

/* from iBCS2 p6-71 */
#ifdef _SCO_CLOCK_T_
typedef	_SCO_CLOCK_T_ clock_t;
#undef _SCO_CLOCK_T_
#endif
#ifdef _SCO_TIME_T_
typedef _SCO_TIME_T_ time_t;
#undef _SCO_TIME_T_
#endif
#ifdef _SCO_SIZE_T_
typedef _SCO_SIZE_T_ size_t;
#undef _SCO_SIZE_T_
#endif

struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};

extern char *tzname[2];
extern long timezone;
extern int daylight;

#define	CLK_TCK		100

/* prototypes required by POSIX.1 */

#include <sys/cdefs.h>

char *asctime __P((const struct tm *));
clock_t clock __P((void));
char *ctime __P((const time_t *));
double difftime __P((time_t, time_t));
struct tm *gmtime __P((const time_t *));
struct tm *localtime __P((const time_t *));
time_t mktime __P((struct tm *));
size_t strftime __P((char *, size_t, const char *, const struct tm *));
time_t time __P((time_t *));

/* for compatibility */
struct timeval {
	long tv_sec;
	long tv_usec;
};

#endif	/* _SCO_SYS_TIME_H_ */
