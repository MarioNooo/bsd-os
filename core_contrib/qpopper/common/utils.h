/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *
 *  06/29/00 [rcg]
 *           - Moved to /common/, made consistent with other
 *             string utils.
 *
 *  02/14/00 [rcg]
 *           - Modified StrNCat0 to make len *not* const.
 *
 *  02/10/00 [rcg]
 *           - Modified StrNCat0 to make len const to
 *             agree with prototype in .h
 *
 * 12/1997   [lgl]
 *           - File added.
 */

#ifndef _UTILSINCLUDED
#  define _UTILSINCLUDED


/*------------   Types   ------------*/

typedef const char *CSTR;
typedef char       *STR;

#define TRUE  1
#define FALSE 0

typedef char BOOL;


#define CHARISSPACE(c)   ( (c) == 32 || (c) == '\t' || (c) == '\r' || (c) == '\n' )

/* BETWEEN : TRUE is val >= min and < max */
#define BETWEEN(val, min, max)   ( ((unsigned) ((val) - (min))) < (unsigned) ((max) - (min)) )


BOOL CharEQI (char c1, char c2);

BOOL CharIsAlpha (char ch);

BOOL CharIsSgmlTok (char ch);

BOOL CharIsAlnum (char ch);

BOOL CharIsDigit (char ch);

int Constrain (int nVal, int nMin, int nMax);

int StrEQI (CSTR s1, CSTR s2);

BOOL StrBeginsI (char *szPrefix, char *sz);

int ScanInt (CSTR *pszEnt);

void StrNCat0 (char *dest, const char *src, int len);

/*
 * <CAUTION>StrNDup(): Acts the same as strdup(); 
 *  required to free the result </CAUTION>
 */
char *StrNDup (char *src, const int len);

#ifdef UNIX
void MemSet (char *dest, unsigned int len, char value);
#endif /* UNIX */

#endif /* _UTILSINCLUDED */
