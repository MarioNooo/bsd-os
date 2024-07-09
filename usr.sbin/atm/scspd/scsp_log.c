/*
 *
 * ===================================
 * HARP  |  Host ATM Research Platform
 * ===================================
 *
 *
 * Copyright (c) 1997-1998, Network Computing Services, Inc.
 * All rights reserved.
 *
 *	@(#) scsp_log.c,v 1.2 1998/07/12 20:49:36 johnc Exp
 *
 */


/*
 * Server Cache Synchronization Protocol (SCSP) Support
 * ----------------------------------------------------
 *
 * SCSP logging routines
 *
 */


#ifndef lint
static char *RCSid = "@(#) scsp_log.c,v 1.2 1998/07/12 20:49:36 johnc Exp";
#endif

#include <netatm/user_include.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/cdefs.h>
#include <sys/ioctl.h>
#include <sys/ttycom.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <arpa/inet.h>
#include <netinet/if_ether.h>

#include "../lib/libatm.h"

#include "scsp_msg.h"
#include "scsp_if.h"
#include "scsp_var.h"

/*
 * Global variables
 */
FILE	*scsp_trace_file = (FILE *)0;


/*
 * Write a message to SCSP's log
 *
 * Arguments:
 *	level	pointer to an SCSP cache key structure
 *	fmt	printf-style format string
 *	...	parameters for printf-style use according to fmt
 *
 * Returns:
 *	none
 *
 */
void
#if __STDC__
scsp_log(const int level, const char *fmt, ...)
#else
scsp_log(level, fmt, va_alist)
	int	level;
	char	*fmt;
	va_dcl
#endif
{
	va_list	ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif

	/*
	 * In debug mode, just write to stdout
	 */
	if (scsp_debug_mode) {
		vprintf(fmt, ap);
		printf("\n");
		return;
	}

	/*
	 * Write to syslog if it's active or if no log file is set up
	 */
	if (scsp_log_syslog || !scsp_log_file) {
		vsyslog(level, fmt, ap);
	}

	/*
	 * Write to the log file if there's one set up
	 */
	if (scsp_log_file) {
		vfprintf(scsp_log_file, fmt, ap);
		fprintf(scsp_log_file, "\n");
	}

	va_end(ap);
}


/*
 * Open SCSP's trace file
 *
 * Arguments:
 *	none
 *
 * Returns:
 *	none
 *
 */
void
scsp_open_trace()
{
	char	fname[64];

	/*
	 * Build a file name
	 */
	UM_ZERO(fname, sizeof(fname));
	sprintf(fname, "/tmp/scspd.%d.trace", getpid());

	/*
	 * Open the trace file.  If the open fails, log an error, but
	 * keep going.  The trace routine will notice that the file
	 * isn't open and won't try to write to it.
	 */
	scsp_trace_file = fopen(fname, "w");
	if (scsp_trace_file == (FILE *)0) {
		scsp_log(LOG_ERR, "Can't open trace file");
	}
}


/*
 * Write a message to SCSP's trace file
 *
 * Arguments:
 *	fmt	printf-style format string
 *	...	parameters for printf-style use according to fmt
 *
 * Returns:
 *	none
 *
 */
void
#if __STDC__
scsp_trace(const char *fmt, ...)
#else
scsp_trace(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif

	/*
	 * Write the message to the trace file, if it's open
	 */
	if (scsp_trace_file) {
		vfprintf(scsp_trace_file, fmt, ap);
	}

	va_end(ap);
}


/*
 * Write an SCSP message to SCSP's trace file
 *
 * Arguments:
 *	dcsp	pointer to DCS block for the message
 *	msg	pointer to the message
 *	dir	a direction indicator--0 for sending, 1 for receiving
 *
 * Returns:
 *	none
 *
 */
void
scsp_trace_msg(dcsp, msg, dir)
	Scsp_dcs	*dcsp;
	Scsp_msg	*msg;
	int		dir;
{
	struct in_addr	addr;

	/*
	 * Copy the remote IP address into a struct in_addr
	 */
	UM_COPY(dcsp->sd_dcsid.id, &addr.s_addr,
			sizeof(struct in_addr));
	
	/*
	 * Write the message to the trace file, if it's open
	 */
	if (scsp_trace_file) {
		scsp_trace("SCSP message at 0x%x %s %s\n",
				(u_long)msg,
				(dir ? "received from" : "sent to"),
				format_ip_addr(&addr));
		print_scsp_msg(scsp_trace_file, msg);
	}
}


/*
 * Log a memory error and exit
 *
 * Arguments:
 *	cp	message to log
 *
 * Returns:
 *	exits, does not return
 *
 */
void
scsp_mem_err(cp)
	char	*cp;
{
	scsp_log(LOG_CRIT, "out of memory: %s", cp);
	exit(2);
}
