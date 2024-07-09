/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 * $Id: lprm.h,v 5.3 2000/04/14 20:40:22 papowell Exp papowell $
 ***************************************************************************/



#ifndef _LPRM_1_
#define _LPRM_1_


EXTERN int All_printers;    /* show all printers */
EXTERN int LP_mode;    /* show all printers */
EXTERN int Mail_fd;    /* show all printers */

/* PROTOTYPES */

void Get_parms( int argc, char **argv );

#endif
