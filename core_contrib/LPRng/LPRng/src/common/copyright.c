/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 *
 ***************************************************************************/

 static char *const _id =
"$Id: copyright.c,v 5.4 2000/04/14 20:39:50 papowell Exp papowell $";


#include "lp.h"
#include "patchlevel.h"
/**** ENDINCLUDE ****/

char *Copyright[] = {
 PATCHLEVEL
#if defined(HAVE_KRB5_H)
 ", Kerberos5"
#endif
#if defined(KRB4_STUFF)
 ", MIT Kerberos4"
#endif
", Copyright 1988-2000 Patrick Powell, <papowell@astart.com>",

"",
"locking uses: "
#ifdef HAVE_FCNTL
		"fcntl (preferred)"
#else
#ifdef HAVE_LOCKF
            "lockf"
#else
            "flock (does NOT work over NFS)"
#endif
#endif
,
"stty uses: "
#if USE_STTY == SGTTYB
            "sgttyb"
#endif
#if USE_STTY == TERMIO
            "termio"
#endif
#if USE_STTY == TERMIOS
            "termios"
#endif
,
"",
#include "license.h"
#include "copyright.h"
0 };
