/**************************************************************************
 * LPRng IFHP Filter
 * Copyright 1994-1999 Patrick Powell, San Diego, CA <papowell@astart.com>
 **************************************************************************/
/**** HEADER *****
$Id: errormsg.h,v 1.2 1999/12/17 02:04:57 papowell Exp papowell $
 **** ENDHEADER ****/

#ifndef _ERRORMSG_H
#define _ERRORMSG_H

/* PROTOTYPES */
const char * Errormsg ( int err );
/* VARARGS2 */
#ifdef HAVE_STDARGS
void logmsg( char *msg,...)
#else
void logmsg(va_alist) va_dcl
#endif
 ;
/* VARARGS2 */
#ifdef HAVE_STDARGS
void logDebug( char *msg,...)
#else
void logDebug(va_alist) va_dcl
#endif
 ;
/* VARARGS2 */
#ifdef HAVE_STDARGS
void fatal ( char *msg,...)
#else
void fatal (va_alist) va_dcl
#endif
 ;
/* VARARGS2 */
#ifdef HAVE_STDARGS
void logerr (char *msg,...)
#else
void logerr (va_alist) va_dcl
#endif
 ;
/* VARARGS2 */
#ifdef HAVE_STDARGS
void logerr_die ( char *msg,...)
#else
void logerr_die (va_alist) va_dcl
#endif
 ;
char *Time_str(int shortform, time_t t);
int Write_fd_str( int fd, char *str );
int Write_fd_len( int fd, char *str, int len );
const char *Sigstr (int n);
void setstatus( char *msg, char *details, int trace );
int udp_open( char *device );

#endif
