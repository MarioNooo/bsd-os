/* ----------------------------------------------------------------------
     MIME Mangler - single pass reduction of MIME to plain text

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright (c) 2003 QUALCOMM Incorporated.

     File: lineend.c - deal with varying end of line conventions
     Version: 0.2.3, Dec 1997
     Last Edited: Oct 29 20:03
   
  ---- */

/* 
  Define the string once and share it to save space 
  */

#include "lineend.h"

#if defined(UNIX) || defined(PILOT)

const char *lineEnd = "\n";

#elif defined (NETWORK) || defined(PC)

const char *lineEnd = "\r\n";

#elif defined (MACINTOSH)

const char *lineEnd = "\r";

#endif

