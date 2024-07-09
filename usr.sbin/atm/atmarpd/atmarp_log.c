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
 *	@(#) atmarp_log.c,v 1.1 1998/07/24 17:11:51 johnc Exp
 *
 */


/*
 * Server Cache Synchronization Protocol (SCSP) Support
 * ----------------------------------------------------
 *
 * SCSP-ATMARP server interface: logging routines
 *
 */


#ifndef lint
static char *RCSid = "@(#) atmarp_log.c,v 1.1 1998/07/24 17:11:51 johnc Exp";
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

#include "../scspd/scsp_msg.h"
#include "../scspd/scsp_if.h"
#include "atmarp_var.h"


/*
 * Write a message to atmarpd's log
 *
 * Arguments:
 *	level	the level (error, info, etc.) of the message
 *	fmt	printf-style format string
 *	...	parameters for printf-style use according to fmt
 *
 * Returns:
 *	none
 *
 */
void
#if __STDC__
atmarp_log(const int level, const char *fmt, ...)
#else
atmarp_log(level, fmt, va_alist)
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
	if (atmarp_debug_mode) {
		vprintf(fmt, ap);
		printf("\n");
		return;
	}

	/*
	 * Check whether we have a log file set up
	 */
	if (!atmarp_log_file) {
		/*
		 * Write to syslog
		 */
		vsyslog(level, fmt, ap);
	} else {
		/*
		 * Write to the log file
		 */
		vfprintf(atmarp_log_file, fmt, ap);
		fprintf(atmarp_log_file, "\n");
	}

	va_end(ap);
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
atmarp_mem_err(cp)
	char	*cp;
{
	atmarp_log(LOG_CRIT, "out of memory: %s", cp);
	exit(2);
}
