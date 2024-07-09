/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions: 
 *
 *  01/05/01  [rcg]
 *            - Added Qstrlen as a safe strlen.
 *
 *  04/21/00  [rcg]
 *            -  Changed snprintf to Qsnprintf, vsnprintf to Qvsnprintf.
 *               added Qsprintf.  Code now unconditionally included.
 *
 *  01/27/00  [rcg]
 *            -  Changed snprintf to vsnprintf, added snprint as jacket.
 *
 *  01/20/00  [rg]
 *            -  File added.
 *
 */



#ifndef _SNPRINTF_H
#  define   _SNPRINTF_H

#  include <sys/types.h>
#  include <stdio.h>
#  include <stdarg.h>

#  include "conf.h"

int Qsnprintf  ( char *s, size_t n, const char *format, ... ) ATTRIB_FMT ( 3, 4 );
int Qsprintf   ( char *s,           const char *format, ... ) ATTRIB_FMT ( 2, 3 );

int Qvsnprintf ( char *s, size_t n, const char *format, va_list ap ) ATTRIB_FMT ( 3, 0 );

long Qstrlen ( const char *s, long m );


#endif  /* _SNPRINTF_H */


