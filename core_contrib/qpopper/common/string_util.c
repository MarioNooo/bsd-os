/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *
 *     11/03/00  [rcg]
 *              - Ensure that string_copy null-terminates.
 *
 *     06/10/00  [rcg]
 *              - File added.
 *
 */
 
 
 /************************************************************
  *                string handling utilities                 *
  ************************************************************/



/*
 * strlcpy and strlcat based on interface described in the paper
 * "strlcpy and strlcat -- consistent, safe, string copy and
 * concatenation" by Todd C. Miller and Theo de Raadt, University
 * of Colorado, Boulder, and OpenBSD project, 1998.
 */


#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "string_util.h"



/*
 * Function:        strlcpy
 *
 * Description:     Copies source string to destination, stopping at null
 *                  or when bufsize -1 characters have been copied.
 *
 * Parameters:      strdest:    pointer to destination buffer.
 *                  strsource:  pointer to source string.
 *                  bufsize:    size of destination buffer.
 *
 * Returns:         Size of source string.  If return value is greater than
 *                  or equal to bufsize, truncation occurred.  Destination
 *                  is always null-terminated (as long as its size > 0).
 */

size_t
strlcpy ( char *strdest, const char *strsource, size_t bufsize )
{
    register char       *dst = strdest;
    register const char *src = strsource;
    register size_t      len = 0;

    /*
     * Copy characters as long as they fit 
     */
    for ( len = bufsize; len > 0; len-- )
    {
        *dst++ = *src;
        if ( *src++ == '\0' )
            break;
    }
    
    /*
     * Add null if we stopped early
     */
    if ( len == 0 && bufsize != 0 )
    {
        *dst = '\0';
    }
    
    /*
     * We always return length of what we would have
     * copied if the buffer were infinite.
     */
    while ( *src != '\0' )
        src++; /* make sure we are at end of source string */ 
    return ( src - strsource );
}


/*
 * Function:        strlcat
 *
 * Description:     Appends source string to destination, stopping at null
 *                  or when bufsize -1 characters have been copied or no
 *                  more space available in destination buffer.
 *
 * Note:            Unlike strncat, size parameter is total size of buffer,
 *                  not remaining space.
 *
 * Parameters:      strdest:    pointer to destination buffer.
 *                  strsource:  pointer to source string.
 *                  bufsize:    total size of destination buffer.
 *
 * Returns:         Size resulting string would be if buffer size was
 *                  infinite.  If return value is greater than or equal to 
 *                  bufsize, truncation occurred.  Destination is always
 *                  null-terminated (as long as its size > 0).
 */

size_t
strlcat ( char *strdest, const char *strsource, size_t bufsize )
{
    register char       *dst    = strdest;
    register const char *src    = strsource;
    register size_t      avail  = 0;
    size_t               dstlen = 0;
    size_t               tmp    = 0;


    if ( bufsize == 0 )
        return strlen(strdest) + strlen(strsource);

    dstlen = strlen ( strdest );
    dst   += dstlen; /* 'Append at the end', Alice might have said. */

    /* 
     * Calculate available space in buffer.
     */
    tmp    = dstlen;
    if ( tmp >= bufsize )
        tmp = bufsize -1;
    avail  = bufsize - tmp;

    /*
     * If no space available, don't copy anything, just return
     * theoretical length of string if we had copied everything.
     */
    if ( avail == 0 )
        return dstlen + strlen(strsource);

    /*
     * Copy what we can.
     */
    while ( *src != '\0' && avail > 1 )
    {
        *dst++ = *src++;
        avail--;
    }
    
    /*
     * Null-terminate and return what we would have copied if
     * the buffer was infinite.  We calculate this by taking
     * the original destination length and adding what we did
     * copy, plus any remaining characters we didn't copy.
     */
    *dst = '\0';
    return dstlen + ( src - strsource ) + strlen ( src );
}


/*
 * Function:        equal_strings
 *
 * Description:     Tests two strings for equality, up to shorter
 *                  of specified strings.  Ignores case.
 *
 * Parameters:      str1:    pointer to first string.
 *                  len1:    length  of first string, or -1 to use strlen.
 *                  str2:    pointer to second string.
 *                  len2:    length  of second string, or -1 to use strlen.
 *
 * Returns:         TRUE if the strings are the same, FALSE otherwise.
 */

BOOL
equal_strings ( char *str1, long len1, char *str2, long len2 )
{
    int         c1 = 0;
    int         c2 = 0;


    if ( len1 == -1 )
        len1 = strlen ( str1 );
    if ( len2 == -1 )
        len2 = strlen ( str2 );
    if ( len1 != len2 )
        return FALSE;

    if ( str1 == str2 && len1 == len2 )
        return TRUE;

    while ( len1 > 0 )
    {
        c1 = *str1;
        c2 = *str2;

        if ( CharEQI ( c1, c2 ) == FALSE )
            return FALSE;

        str1++;
        str2++;
        len1--;
    }

    return TRUE;
}


char *
string_copy ( const char *str, size_t len )
{
    char *buf;
    char *q;

    buf = malloc ( len + 1 );
    if ( buf != NULL )
    {
        q = buf;
        while ( len > 0 )
        {
            *q++ = *str++;
            len--;
        }

    *q = '\0';
    }

    return buf;
}

