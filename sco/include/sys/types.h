/*	BSDI types.h,v 1.1 1995/07/10 18:26:33 donn Exp	*/

#ifndef _SCO_SYS_TYPES_H_
#define	_SCO_SYS_TYPES_H_

/* from iBCS2 p6-81 */

#include <machine/ansi.h>

#ifdef _SCO_CLOCK_T_
typedef	_SCO_CLOCK_T_ clock_t;
#undef _SCO_CLOCK_T_
#endif
#ifdef _SCO_SIZE_T_
typedef _SCO_SIZE_T_ size_t;
#undef _SCO_SIZE_T_
#endif
#ifdef _SCO_TIME_T_
typedef _SCO_TIME_T_ time_t;
#undef _SCO_TIME_T_
#endif

typedef char *caddr_t;
typedef long daddr_t;
typedef short dev_t;
typedef unsigned short gid_t;
typedef unsigned short ino_t;
typedef long key_t;
typedef short nlink_t;
typedef long off_t;
typedef unsigned short uid_t;

typedef unsigned char uchar_t;
typedef unsigned char u_char;
typedef unsigned short ushort;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long ulong_t;
typedef unsigned long ulong;
typedef unsigned long u_long;

#endif	/* _SCO_SYS_TYPES_H_ */
