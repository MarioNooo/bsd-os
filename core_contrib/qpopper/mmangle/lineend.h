/* ----------------------------------------------------------------------
     MIME Mangler - single pass reduction of MIME to plain text

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright (c) 2003 QUALCOMM Incorporated.

     File: lineend.h - deal with varying end of line conventions
     Version: 0.2.3, Dec 1997
     Last Edited: Nov 10 22:35
   
  ---- */

#ifndef _LINEENDINCLUDED
#define _LINEENDINCLUDED
#include "config.h"
extern const char *lineEnd;

/*#undef UNIX */
/*#undef PILOT */
/* #define NETWORK */

#if defined(UNIX) || defined(PILOT)

#define EOLCHECK(string) (*(string) == '\n')
#define ENDLINE(string)  (*(string)++ = '\n')
#define EOLLEN           (1)      
#define EOLSTRING        ("\n")
#define EOLCHAR          '\n'

#elif defined(NETWORK) || defined(PC)

#define EOLCHECK(string) (*string == '\r')
#define ENDLINE(string) (*(string)++ = '\r', *(string)++ = '\n' )
#define EOLLEN           (2)   
#define EOLSTRING        ("\r\n")   

#elif defined(MACINTOSH)

#define EOLCHECK(string) (*string == '\r')
#define ENDLINE(string) (*(string)++ = '\r')
#define EOLLEN           (1)      
#define EOLSTRING        ("\r")
#define EOLCHAR          '\r'

#endif

#endif /* _LINEENDINCLUDED */
