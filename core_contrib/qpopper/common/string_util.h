/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *
 *     06/28/00  [rcg]
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

#ifndef _STRING_UTIL_H
#  define _STRING_UTIL_H


#include <sys/types.h>
#include <string.h>

#include "utils.h"



size_t strlcpy(char *strdest, const char *strsource, size_t bufsize);
size_t strlcat(char *strdest, const char *strsource, size_t bufsize);
BOOL   equal_strings(char *str1, long len1, char *str2, long len2);
char  *string_copy (const char *str, size_t len);


#endif /* STRING_UTIL_H */
