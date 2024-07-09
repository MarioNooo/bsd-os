/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 */

/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>

/* 
 *  lower:  Convert a string to lowercase
 */

void
pop_lower ( buf )
char    *   buf;
{
    char        *   mp;

    for ( mp = buf; *mp != '\0'; mp++ )
        if ( isupper ( (int) *mp ) )
            *mp = (char) tolower ( (int)*mp );
}
