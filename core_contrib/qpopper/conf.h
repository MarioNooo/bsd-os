/*
 * Copyright (c) 2000 Qualcomm Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 */

#ifndef _CONF_H_
#define _CONF_H_


#ifndef __PROTO
# if defined (__STDC__)
#  define __PROTO(z) z
# else
#  define __PROTO(z) ()
# endif 
#endif /* !__PROTO */


#ifdef    SUPPORT_ATTRIBUTE_FORMAT
#  define ATTRIB_FMT(A,B) __attribute__ ((format (printf, A, B)))
#else  /* not SUPPORT_ATTRIBUTE_FORMAT */
#  define ATTRIB_FMT(A,B)
#endif /* SUPPORT_ATTRIBUTE_FORMAT */


#endif /* !_CONF_H_ */
