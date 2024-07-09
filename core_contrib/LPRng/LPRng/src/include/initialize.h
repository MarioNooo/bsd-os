/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 * $Id: initialize.h,v 5.3 2000/04/14 20:40:17 papowell Exp papowell $
 ***************************************************************************/



#ifndef _INITIALIZE_H
#define _INITIALIZE_H

/* PROTOTYPES */

void Initialize( int argc, char *argv[], char *envp[] ); 
void Setup_configuration( void );
char *Get_user_information( void );
 
#endif
