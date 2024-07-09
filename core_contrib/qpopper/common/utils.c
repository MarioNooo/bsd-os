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


#ifndef UNIX
#  include <Pilot.h>
#else /* UNIX */

#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>

#  define StrCompare strcmp
#  define StrCopy    strcpy
#  define StrCat     strcat
#  define StrLen     strlen

#endif /* UNIX */


#include "utils.h"


BOOL CharEQI( char c1, char c2 ) { 
   if ( c1 == c2 )
      return TRUE;
   c1 |= 32;
   return (BOOL) ( BETWEEN ( c1, 'a', 'z' +1 )  &&  c1 == ( c2 | 32 ) );
}


BOOL CharIsAlpha ( char ch ) {
   return (BOOL) BETWEEN ( ( ch | 32 ), 'a', 'z' + 1 );
}


BOOL CharIsSgmlTok ( char ch ) {
  return (BOOL) (   BETWEEN ( ch | 32, 'a', 'z' + 1 )
                 || BETWEEN ( ch, '0', '9' +1 )
                 || ch == '@'
                 || ch == '_'
                 || ch == '.'
                 || ch == '-'
                 || ch == '+'
                 || ch == '$');
}



/*
||  If the compiler doesn't support inline functions, these are better as macros:
*/

BOOL
CharIsAlnum ( char ch ) 
{
   return (BOOL) (   BETWEEN ( ch, '0', '9' +1 )
                  || BETWEEN ( ch | 32, 'a', 'z' +1 ) );

}


int
Constrain ( int nVal, int nMin, int nMax ) 
{
   return ( nVal < nMin ? nMin :
            nVal > nMax ? nMax :
            nVal );
}


/* Return TRUE if s1 and s2 are equal */
int
StrEQI ( CSTR s1, CSTR s2 ) 
{
  if ( s1 == s2 )
    return ( TRUE );

   while ( CharEQI ( *s1, *s2 ) ) {
      if ( *s1 == 0 || *s2 == 0 )
         return ( *s1 == *s2 );
      ++s1;
      ++s2;
   }
   return ( CharEQI ( *s1, *s2 ) );
}


BOOL
StrBeginsI ( char *szPrefix, char *sz ) 
{
   char ch;
   while ( 0 != ( ch = *szPrefix ) ) {
      if ( !CharEQI ( ch, *sz ) )
         return FALSE;

      ++szPrefix;
      ++sz;
   }
   return TRUE;
}


/*
|| Read an integer at (*pszEnt), setting *pszEnt to the character
|| following the integer.
|| Return the value of the integer.
*/
int
ScanInt ( CSTR *pszEnt )  
{
   CSTR szEnt = *pszEnt;
   int n = 0;

   while ( BETWEEN ( *szEnt, '0', '9' + 1 ) ) {
      n = n * 10 + *szEnt - '0';
      ++szEnt;
   }
   *pszEnt = szEnt;
   return n;
}



void
StrNCat0 ( char *dest, const char *src, int len )
{
  while ( *dest )
    dest++; 
  while ( len-- && ( *dest++ = *src++ ) );
  *dest = '\0';
}

char
*StrNDup ( char *src, const int len )
{
    char *local = (char *) malloc ( len+1 );
    if ( local ) {
        strncpy ( local, src, len );
        local[len] = '\0';
    }
    return local;
}

#ifdef UNIX
void
MemSet ( char *dest, unsigned int len, char value )
{
  char *destEnd = dest + len;
  while ( dest != destEnd )
    *dest++ = value;
}
#endif


