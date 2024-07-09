/* 
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 */

/* 
 * Revisions:
 *
 * 08/19/00  [rg]
 *         - Added vlogit.
 *
 * 01/25/00  [rg]
 *        - Added parameters for file name and line number.
 *
 * 12/22/99  [rg]
 *        - Changed __STDC__ to STDC_HEADERS, because sometimes
 *          __STDC__ is only defined if the compiler is invoked
 *          with flags which disable vendor extensions and force
 *          pure ANSI compliance; the intent of its use here is
 *          to check for modern compilers.
 *
 * 04/21/00  [rg]
 *        - Removed STDC_HEADERS/K&R junk.
 *
 * 12/02/99  [rg]
 *        - Added DEBUGING macros to avoid extra #ifdefs
 *
 *  9/16/98  [rg]
 *         - File added.
 *
 */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdarg.h>

#include "popper.h"


int logit ( FILE        *str,       /* STREAM to write to, or NULL for syslog */
            log_level    loglev,    /* log level */
            const char  *fn,        /* file name whence called */
            size_t       ln,        /* line number whence called */
            const char  *format,    /* format string to log */
            ... );                  /* parameters for format string */

int vlogit ( FILE        *str,       /* STREAM to write to, or NULL for syslog */
             log_level    loglev,    /* log level */
             const char  *fn,        /* file name whence called */
             size_t       ln,        /* line number whence called */
             const char  *format,    /* format string to log */
             va_list      ap );  

