/*-
 * Copyright (c) 1993, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI pathnames.h,v 2.4 2002/02/08 23:32:28 polk Exp
 */

#include <paths.h>

/*
 * XXX
 * Use relative pathnames and expect them to be in the users path.
 * Security problem?
 */
#define	_PATH_CAT		"/bin/cat"
#define	_PATH_DD		"/bin/dd"
#define	_PATH_ECHO		"/bin/echo"
#define	_PATH_LOG		"/var/db/install.log"
#define	_PATH_MKDIR		"/bin/mkdir"
#define	_PATH_MORE		"/usr/bin/more"
#define	_PATH_MT		"/bin/mt"
#define	_PATH_PAX		"/bin/pax"
#define	_PATH_RM		"/bin/rm"
#ifndef	_PATH_UNCOMPRESS
#define	_PATH_UNCOMPRESS	"/usr/bin/zcat"
#endif
#define	_PATH_UNZIP		"/usr/bin/gzcat"
