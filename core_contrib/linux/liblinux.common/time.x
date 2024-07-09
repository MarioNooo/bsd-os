/*	BSDI time.x,v 1.4 2002/11/19 20:37:53 donn Exp	*/

/*
 * Transforms for time.h functions.
 */

include "types.xh"

%{
#include <sys/param.h>
#include <sys/times.h> 
#include <time.h>
#include <utime.h>
#include <sys/resource.h>
%}

int gettimeofday(struct timeval *tv, struct timezone *tz);
int settimeofday(const struct timeval *tv, const struct timezone *tz);

int getitimer(int timer, struct itimerval *itv);
int setitimer(int timer, const struct itimerval *itv, const struct itimerval *otv);

int stime(const time_t *t)
{
	struct timeval tv;
	struct timeval *tvp;

	if (t) {
		tvp = &tv;
		tv.tv_sec = *t;
		tv.tv_usec = 0;
	} else
		tvp = NULL;
	return (__bsdi_syscall(SYS_settimeofday, tvp, NULL));
}

/* Adapted from BSD time() routine.  */
time_t time(time_t *t)
{
	struct timeval tv;
	int r;

	if (__bsdi_error(r = __bsdi_syscall(SYS_gettimeofday, &tv, NULL)))
		return (r);
	if (t)
		*t = tv.tv_sec;
	return (tv.tv_sec);
}

/* Adapted from BSD utime() routine.  */
int utime(const char *path, struct utimbuf *ut)
{
	struct timeval tva[2];
	struct timeval *tv;

	if (ut) {
		tva[0].tv_sec = ut->actime;
		tva[0].tv_usec = 0;
		tva[1].tv_sec = ut->modtime;
		tva[1].tv_usec = 0;
	} else
		tv = NULL;
	return (__bsdi_syscall(SYS_utimes, path, tv));
}

/* Linux emulates utimes() using utime().  We provide BSD utimes().  */
int utimes(const char *path, struct timeval *tv);

/* Adapted from BSD times() routine.  */
clock_t times(struct tms *tp)
{
	struct rusage ru;
	struct timeval t;
	int r;

#define CONVTCK(r)      (r.tv_sec * CLK_TCK + r.tv_usec / (1000000 / CLK_TCK))

	if (__bsdi_error(r = __bsdi_syscall(SYS_getrusage, RUSAGE_SELF, &ru)))
		return ((clock_t)r);
	tp->tms_utime = CONVTCK(ru.ru_utime);
	tp->tms_stime = CONVTCK(ru.ru_stime);
	if (__bsdi_error(r = __bsdi_syscall(SYS_getrusage, RUSAGE_CHILDREN,
	    &ru)))
		return ((clock_t)r);
	tp->tms_cutime = CONVTCK(ru.ru_utime);
	tp->tms_cstime = CONVTCK(ru.ru_stime);
	if (__bsdi_error(r = __bsdi_syscall(SYS_gettimeofday, &t, NULL)))
		return ((clock_t)r);
	return ((clock_t)(CONVTCK(t)));
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

/*
 * XXX adjtime()
 * XXX adjtimex()
 */
