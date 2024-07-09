/**************************************************************************
 * LPRng IFHP Filter
 * Copyright 1994-1999 Patrick Powell, San Diego, CA <papowell@astart.com>
 **************************************************************************/
/**** HEADER *****
plp_snprintf.h,v 1.3 2000/04/19 03:18:32 jch Exp
 **** HEADER *****/

#if !defined(_PLP_SNPRINTF_)
#define _PLP_SNPRINTF_ 1
/* PROTOTYPES */
int plp_vsnprintf(char *str, size_t count, const char *fmt, va_list args);
/* VARARGS3 */
#ifdef HAVE_STDARGS
int plp_snprintf (char *str,size_t count,const char *fmt,...)
#else
int plp_snprintf (va_alist) va_dcl
#endif
 ;

#endif
