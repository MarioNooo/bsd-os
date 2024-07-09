/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 *
 ***************************************************************************/

 static char *const _id =
"$Id: errormsg.c,v 5.7 2000/04/14 20:39:50 papowell Exp papowell $";


#include "lp.h"
#include "errormsg.h"
#include "errorcodes.h"
#include "child.h"
#include "getopt.h"
/**** ENDINCLUDE ****/

#if defined(HAVE_SYSLOG_H)
# include <syslog.h>
#endif

/****************************************************************************/


 static void use_syslog( int kind, char *msg );

/****************************************************************************
 * char *Errormsg( int err )
 *  returns a printable form of the
 *  errormessage corresponding to the valie of err.
 *  This is the poor man's version of sperror(), not available on all systems
 *  Patrick Powell Tue Apr 11 08:05:05 PDT 1995
 ****************************************************************************/
/****************************************************************************/
#if !defined(HAVE_STRERROR)

# if defined(HAVE_SYS_NERR)
#  if !defined(HAVE_SYS_NERR_DEF)
     extern int sys_nerr;
#  endif
#  define num_errors    (sys_nerr)
# else
#  define num_errors    (-1)            /* always use "errno=%d" */
# endif

# if defined(HAVE_SYS_ERRLIST)
#  if !defined(HAVE_SYS_ERRLIST_DEF)
     extern const char *const sys_errlist[];
#  endif
# else
#  undef  num_errors
#  define num_errors   (-1)            /* always use "errno=%d" */
# endif

#endif


const char * Errormsg ( int err )
{
    const char *cp;

	if( err == 0 ){
		cp = "No Error";
	} else {
#if defined(HAVE_STRERROR)
		cp = strerror(err);
#else
# if defined(HAVE_SYS_ERRLIST)
		if (err >= 0 && err < num_errors) {
			cp = sys_errlist[err];
		} else
# endif
		{
			static char msgbuf[32];     /* holds "errno=%d". */
			/* SAFE use of sprintf */
			(void) sprintf (msgbuf, "errno=%d", err);
			cp = msgbuf;
		}
#endif
	}
    return (cp);
}

 struct msgkind {
    int var;
    char *str;
};

 static struct msgkind msg_name[] = {
    {LOG_CRIT, " (CRIT)"},
    {LOG_ERR, " (ERR)"},
    {LOG_WARNING, " (WARN)"},
    {LOG_NOTICE, " (NOTICE)"},
    {LOG_INFO, " (INFO)"},
    {LOG_DEBUG, ""},
    {0}
};

 static char * putlogmsg(int kind)
{
    int i;
    static char b[32];

	b[0] = 0;
	if( kind < 0 ) return(b);
    for (i = 0; msg_name[i].str; ++i) {
		if ( msg_name[i].var == kind) {
			return (msg_name[i].str);
		}
    }
	/* SAFE USE of SNPRINTF */
    (void) SNPRINTF (b, sizeof(b), "<BAD LOG FLAG %d>", kind);
    return (b);
}


 static void use_syslog(int kind, char *msg)
{
    /* testing mode indicates that this is not being used
     * in the "real world", so don't get noisy. */

#ifndef HAVE_SYSLOG_H
	/* Note: some people would open "/dev/console", as default
		Bad programmer, BAD!  You should parameterize this
		and set it up as a default value.  This greatly aids
		in testing for portability.
		Patrick Powell Tue Apr 11 08:07:47 PDT 1995
	 */
	int Syslog_fd;
	if	(Syslog_fd = open( Syslog_device_DYN,
				O_WRONLY|O_APPEND|O_NOCTTY, Spool_file_perms_DYN )) > 0 ) ){
		int len;

		len = strlen(msg);
		msg[len] = '\n';
		msg[len+1] = 0;
		Write_fd_len( Syslog_fd, msg, len+1 );
		close( Syslog_fd );
		msg[len] = 0;
    }

#else                           /* HAVE_SYSLOG_H */
# ifdef HAVE_OPENLOG
	/* use the openlog facility */
	openlog(Name, LOG_PID | LOG_NOWAIT, SYSLOG_FACILITY );
	syslog(kind, "%s", msg);
	closelog();

# else
    (void) syslog(SYSLOG_FACILITY | kind, "%s", msg);
# endif							/* HAVE_OPENLOG */
#endif                          /* HAVE_SYSLOG_H */
}


 static void log_backend (int kind, char *log_buf)
{
    char stamp_buf[2*SMALLBUFFER];
	int n;
	char *s;
	/* plp_block_mask omask; / **/
	int err = errno;

    /* plp_block_all_signals (&omask); / **/
	/* put the error message into the status file as well */
    /*
     * Now build up the state info: timestamp, hostname, argv[0], pid, etc.
     * Don't worry about efficiency here -- it shouldn't happen very often
     * (unless we're debugging).
     */

	stamp_buf[0] = 0;
	/* remove trailing \n */
	if( (s = strrchr(log_buf, '\n')) && cval(s+1) == 0 ){
		*s = 0;
	}


	if( Is_server || DEBUGL1 ){
		/* log stuff */
		if( (LOG_EMERG < LOG_INFO && kind <= LOG_INFO )
			|| ( LOG_EMERG > LOG_INFO && kind >= LOG_INFO )){
			setstatus( 0, "%s", log_buf );
			use_syslog(kind, log_buf);
		}
		n = strlen(stamp_buf); s = stamp_buf+n; n = sizeof(stamp_buf)-n;
		(void) SNPRINTF( s, n, "%s", Time_str(0,0) );
		if (ShortHost_FQDN ) {
			n = strlen(stamp_buf); s = stamp_buf+n; n = sizeof(stamp_buf)-n;
			(void) SNPRINTF( s, n, " %s", ShortHost_FQDN );
		}
		if(Debug || DbgFlag){
			n = strlen(stamp_buf); s = stamp_buf+n; n = sizeof(stamp_buf)-n;
			(void) SNPRINTF(s, n, " [%d]", getpid());
			n = strlen(stamp_buf); s = stamp_buf+n; n = sizeof(stamp_buf)-n;
			if(Name) (void) SNPRINTF(s, n, " %s", Name);
			n = strlen(stamp_buf); s = stamp_buf+n; n = sizeof(stamp_buf)-n;
			(void) SNPRINTF(s, n, " %s", putlogmsg(kind) );
		}
		n = strlen(stamp_buf); s = stamp_buf+n; n = sizeof(stamp_buf)-n;
		(void) SNPRINTF(s, n, " %s", log_buf );
	} else {
		(void) SNPRINTF(stamp_buf, sizeof(stamp_buf), "%s", log_buf );
	}

	if( strlen(stamp_buf) > sizeof(stamp_buf) - 8 ){
		stamp_buf[sizeof(stamp_buf)-8] = 0;
		strcpy(stamp_buf+strlen(stamp_buf),"...");
	}
	n = strlen(stamp_buf); s = stamp_buf+n; n = sizeof(stamp_buf)-n;
	(void) SNPRINTF(s, n, "\n" );


    /* use writes here: on HP/UX, using f p rintf really screws up. */
	/* if stdout or stderr is dead, you have big problems! */

	Write_fd_str( 2, stamp_buf );

    /* plp_unblock_all_signals ( &omask ); / **/
	errno = err;
}

/*****************************************************
 * Put the printer name at the start of the log buffer
 *****************************************************/
 
 static void prefix_printer( char *log_buf, int len )
{
	log_buf[0] = 0;
    if( Printer_DYN ){
		SNPRINTF( log_buf, len-4, "%s: ", Printer_DYN );
	}
}

/* VARARGS2 */
#ifdef HAVE_STDARGS
 void logmsg(int kind, char *msg,...)
#else
 void logmsg(va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    int kind;
    char *msg;
#endif
	int n;
	char *s;
	int err = errno;
	static int in_log;
	char log_buf[SMALLBUFFER];
    VA_LOCAL_DECL
    VA_START (msg);
    VA_SHIFT (kind, int);
    VA_SHIFT (msg, char *);

	if( in_log == 0 ){
		++in_log;
		prefix_printer(log_buf, sizeof(log_buf));
		n = strlen(log_buf); s = log_buf+n; n = sizeof(log_buf)-n-4;
		(void) VSNPRINTF(s, n, msg, ap);
		log_backend (kind,log_buf);
		in_log = 0;
	}
    VA_END;
	errno = err;
}

/* VARARGS2 */
#ifdef HAVE_STDARGS
 void fatal (int kind, char *msg,...)
#else
 void fatal (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    int kind;
    char *msg;
#endif
	int n;
	char *s;
	char log_buf[SMALLBUFFER];
	static int in_log;
    VA_LOCAL_DECL
    VA_START (msg);
    VA_SHIFT (kind, int);
    VA_SHIFT (msg, char *);
	
	if( in_log == 0 ){
		++in_log;
		prefix_printer(log_buf, sizeof(log_buf));
		n = strlen(log_buf); s = log_buf+n; n = sizeof(log_buf)-n-4;
		(void) VSNPRINTF(s, n, msg, ap);
		log_backend (kind, log_buf);
		in_log = 0;
	}

    VA_END;
    cleanup(0);
}

/* VARARGS2 */
#ifdef HAVE_STDARGS
 void logerr (int kind, char *msg,...)
#else
 void logerr (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    int kind;
    char *msg;
#endif
	int err = errno;
	char log_buf[SMALLBUFFER];
	static int in_log;
    VA_LOCAL_DECL
    VA_START (msg);
    VA_SHIFT (kind, int);
    VA_SHIFT (msg, char *);

	if( in_log == 0 ){
		int n;
		char *s;
		++in_log;
		prefix_printer(log_buf, sizeof(log_buf)-4);
		n = strlen(log_buf); s = log_buf+n; n = sizeof(log_buf)-n-4;
		(void) VSNPRINTF(s, n, msg, ap);
		n = strlen(log_buf); s = log_buf+n; n = sizeof(log_buf)-n-4;
		if( err ) (void) SNPRINTF (s, n, " - %s", Errormsg (err));
		log_backend (kind, log_buf);
		in_log = 0;
	}
    VA_END;
    errno = err;
}

/* VARARGS2 */
#ifdef HAVE_STDARGS
 void logerr_die (int kind, char *msg,...)
#else
 void logerr_die (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    int kind;
    char *msg;
#endif
	int err = errno;
	char log_buf[SMALLBUFFER];
	static int in_log;
    VA_LOCAL_DECL

    VA_START (msg);
    VA_SHIFT (kind, int);
    VA_SHIFT (msg, char *);

	if( in_log == 0 ){
		int n;
		char *s;
		++in_log;
		prefix_printer(log_buf, sizeof(log_buf));
		n = strlen(log_buf); s = log_buf+n; n = sizeof(log_buf)-n;
		(void) VSNPRINTF (s, n, msg, ap);
		n = strlen(log_buf); s = log_buf+n; n = sizeof(log_buf)-n;
		if( err ) (void) SNPRINTF (s, n, " - %s", Errormsg (err));
		log_backend (kind, log_buf);
		in_log = 0;
	}
    cleanup(0);
    VA_END;
}

/***************************************************************************
 * Diemsg( char *m1, *m2, ...)
 * print error message to stderr, and die
 ***************************************************************************/

/* VARARGS1 */
#ifdef HAVE_STDARGS
 void Diemsg (char *msg,...)
#else
 void Diemsg (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    char *msg;
#endif
	char buffer[SMALLBUFFER];
	char *s;
	int n;
	static int in_log;
    VA_LOCAL_DECL

    VA_START (msg);
    VA_SHIFT (msg, char *);
	if( in_log == 0 ){
		++in_log;
		buffer[0] = 0;
		n = strlen(buffer); s = buffer + n; n = sizeof(buffer) - n;
		(void) SNPRINTF(s, n, "Fatal error - ");

		n = strlen(buffer); s = buffer + n; n = sizeof(buffer) - n;
		(void) VSNPRINTF (s, n, msg, ap);
		n = strlen(buffer); s = buffer + n; n = sizeof(buffer) - n;
		(void) SNPRINTF (s, n, "\n" );

		/* ignore error, we are dying anyways */
		Write_fd_str( 2, buffer );
		in_log = 0;
	}

    cleanup(0);
    VA_END;
}

/***************************************************************************
 * Warnmsg( char *m1, *m2, ...)
 * print warning message to stderr
 ***************************************************************************/

/* VARARGS1 */
#ifdef HAVE_STDARGS
 void Warnmsg (char *msg,...)
#else
 void Warnmsg (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    char *msg;
#endif
	char buffer[SMALLBUFFER];
	char *s = buffer;
	int n;
	int err = errno;
	static int in_log;
    VA_LOCAL_DECL
    VA_START (msg);
    VA_SHIFT (msg, char *);

	if( in_log ) return;
	++in_log;
	buffer[0] = 0;
	n = strlen(buffer); s = buffer+n; n = sizeof(buffer)-n;
    (void) SNPRINTF(s, n, "Warning - ");
	n = strlen(buffer); s = buffer+n; n = sizeof(buffer)-n;
    (void) VSNPRINTF (s, n, msg, ap);
	n = strlen(buffer); s = buffer+n; n = sizeof(buffer)-n;
    (void) SNPRINTF(s, n, "\n");

	Write_fd_str( 2, buffer );
	in_log = 0;
	errno = err;
    VA_END;
}


/***************************************************************************
 * Warnmsg( char *m1, *m2, ...)
 * print warning message to stderr
 ***************************************************************************/

/* VARARGS1 */
#ifdef HAVE_STDARGS
 void Msg (char *msg,...)
#else
 void Msg (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    char *msg;
#endif
	char buffer[SMALLBUFFER];
	char *s = buffer;
	int n;
	int err = errno;
	static int in_log;
    VA_LOCAL_DECL
    VA_START (msg);
    VA_SHIFT (msg, char *);

	if( in_log ) return;
	++in_log;
	buffer[0] = 0;
	n = strlen(buffer); s = buffer+n; n = sizeof(buffer)-n;
    (void) VSNPRINTF (s, n, msg, ap);
	n = strlen(buffer); s = buffer+n; n = sizeof(buffer)-n;
    (void) SNPRINTF(s, n, "\n");

	Write_fd_str( 2, buffer );
	in_log = 0;
	errno = err;
    VA_END;
}

/* VARARGS1 */
#ifdef HAVE_STDARGS
 void logDebug (char *msg,...)
#else
 void logDebug (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    char *msg;
#endif
	int err = errno;
	char log_buf[SMALLBUFFER];
	static int in_log;
    VA_LOCAL_DECL
    VA_START (msg);
    VA_SHIFT (msg, char *);

	if( in_log == 0 ){
		int n;
		char *s;

		++in_log;
		prefix_printer(log_buf, sizeof(log_buf));
		n = strlen(log_buf); s = log_buf+n; n = sizeof(log_buf)-n;
		(void) VSNPRINTF(s, n, msg, ap);
		log_backend(LOG_DEBUG, log_buf);
		in_log = 0;
	}
	errno = err;
    VA_END;
}

/***************************************************************************
 * char *Sigstr(n)
 * Return a printable form the the signal
 ***************************************************************************/

 struct signame {
    char *str;
    int value;
};

#undef PAIR
#ifndef _UNPROTO_
# define PAIR(X) { #X , X }
#else
# define __string(X) "X"
# define PAIR(X) { __string(X) , X }
#endif

 struct signame signals[] = {
{ "NO SIGNAL", 0 },
#ifdef SIGHUP
PAIR(SIGHUP),
#endif
#ifdef SIGINT
PAIR(SIGINT),
#endif
#ifdef SIGQUIT
PAIR(SIGQUIT),
#endif
#ifdef SIGILL
PAIR(SIGILL),
#endif
#ifdef SIGTRAP
PAIR(SIGTRAP),
#endif
#ifdef SIGIOT
PAIR(SIGIOT),
#endif
#ifdef SIGABRT
PAIR(SIGABRT),
#endif
#ifdef SIGEMT
PAIR(SIGEMT),
#endif
#ifdef SIGFPE
PAIR(SIGFPE),
#endif
#ifdef SIGKILL
PAIR(SIGKILL),
#endif
#ifdef SIGBUS
PAIR(SIGBUS),
#endif
#ifdef SIGSEGV
PAIR(SIGSEGV),
#endif
#ifdef SIGSYS
PAIR(SIGSYS),
#endif
#ifdef SIGPIPE
PAIR(SIGPIPE),
#endif
#ifdef SIGALRM
PAIR(SIGALRM),
#endif
#ifdef SIGTERM
PAIR(SIGTERM),
#endif
#ifdef SIGURG
PAIR(SIGURG),
#endif
#ifdef SIGSTOP
PAIR(SIGSTOP),
#endif
#ifdef SIGTSTP
PAIR(SIGTSTP),
#endif
#ifdef SIGCONT
PAIR(SIGCONT),
#endif
#ifdef SIGCHLD
PAIR(SIGCHLD),
#endif
#ifdef SIGCLD
PAIR(SIGCLD),
#endif
#ifdef SIGTTIN
PAIR(SIGTTIN),
#endif
#ifdef SIGTTOU
PAIR(SIGTTOU),
#endif
#ifdef SIGIO
PAIR(SIGIO),
#endif
#ifdef SIGPOLL
PAIR(SIGPOLL),
#endif
#ifdef SIGXCPU
PAIR(SIGXCPU),
#endif
#ifdef SIGXFSZ
PAIR(SIGXFSZ),
#endif
#ifdef SIGVTALRM
PAIR(SIGVTALRM),
#endif
#ifdef SIGPROF
PAIR(SIGPROF),
#endif
#ifdef SIGWINCH
PAIR(SIGWINCH),
#endif
#ifdef SIGLOST
PAIR(SIGLOST),
#endif
#ifdef SIGUSR1
PAIR(SIGUSR1),
#endif
#ifdef SIGUSR2
PAIR(SIGUSR2),
#endif
{0,0}
    /* that's all */
};

#ifndef NSIG
# define  NSIG 32
#endif


const char *Sigstr (int n)
{
    static char buf[40];
	const char *s = 0;

	if( n == 0 ){
		return( "No signal");
	}
    if (n < NSIG && n >= 0) {
#if defined(HAVE_SYS_SIGLIST)
# if !defined(HAVE_SYS_SIGLIST_DEF)
		extern const char *sys_siglist[];
# endif
		s = sys_siglist[n];
#else
# if defined(HAVE__SYS_SIGLIST)
#  if !defined(HAVE__SYS_SIGLIST_DEF)
		extern const char *_sys_siglist[];
#  endif
		s = _sys_siglist[n];
# else
		int i;
		for( i = 0; signals[i].str && signals[i].value != n; ++i );
		s = signals[i].str;
# endif
#endif
	}
	if( s == 0 ){
		s = buf;
		(void) SNPRINTF (buf, sizeof(buf), "signal %d", n);
	}
    return(s);
}

/***************************************************************************
 * Decode_status (plp_status_t *status)
 * returns a printable string encoding return status
 ***************************************************************************/

const char *Decode_status (plp_status_t *status)
{
    static char msg[LINEBUFFER];

	int n;
    *msg = 0;		/* just in case! */
    if (WIFEXITED (*status)) {
		n = WEXITSTATUS(*status);
		if( n > 0 && n < 32 ) n += JFAIL-1;
		(void) SNPRINTF (msg, sizeof(msg),
		"exit status %d (%s)", WEXITSTATUS(*status),
				 Server_status(n) );
    } else if (WIFSTOPPED (*status)) {
		(void) strcpy(msg, "stopped");
    } else {
		(void) SNPRINTF (msg, sizeof(msg), "died%s",
			WCOREDUMP (*status) ? " and dumped core" : "");
		if (WTERMSIG (*status)) {
			(void) SNPRINTF(msg + strlen (msg), sizeof(msg)-strlen(msg),
				 ", %s", Sigstr ((int) WTERMSIG (*status)));
		}
    }
    return (msg);
}

/***************************************************************************
 * char *Server_status( int d )
 *  translate the server status;
 ***************************************************************************/

	static struct signame statname[] = {
	PAIR(JSUCC),
	PAIR(JFAIL),
	PAIR(JABORT),
	PAIR(JREMOVE),
	PAIR(JACTIVE),
	PAIR(JIGNORE),
	PAIR(JHOLD),
	PAIR(JNOSPOOL),
	PAIR(JNOPRINT),
	PAIR(JSIGNAL),
	PAIR(JFAILNORETRY),
	{0,0}
	};

char *Server_status( int d )
{
	char *s;
	int i;
	static char msg[LINEBUFFER];

	for( i = 0; (s = statname[i].str) && statname[i].value != d; ++i );
	if( s == 0 ){
		s = msg;
		if( d > 0 && d < 32 ){
			SNPRINTF( msg, sizeof(msg), "TERMINATED SIGNAL %s",
				Sigstr( d ) );
		} else {
			SNPRINTF( msg, sizeof(msg), "UNKNOWN STATUS '%d'", d );
		}
	}
	return(s);
}