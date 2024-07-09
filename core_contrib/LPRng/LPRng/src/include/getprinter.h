/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 * $Id: getprinter.h,v 5.5 2000/04/14 20:40:16 papowell Exp papowell $
 ***************************************************************************/




#ifndef _GETPRINTER_H_
#define _GETPRINTER_H_ 1

/* PROTOTYPES */
char *Get_printer(void);
void Fix_Rm_Rp_info(char *report_conflict, int report_len );
void Get_all_printcap_entries(void);

#endif
