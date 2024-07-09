/*	BSDI limits.h,v 1.1 1995/07/10 18:21:10 donn Exp	*/

#ifndef _SCO_LIMITS_H_
#define	_SCO_LIMITS_H_

/* from iBCS2 p6-40 - 6-41 */
#define	CHAR_BIT	8
#define	SCHAR_MAX	127
#define	SCHAR_MIN	(-128)
#define	UCHAR_MAX	255
#define	CHAR_MAX	0
#define	CHAR_MIN	255
#define	SHRT_MAX	32767
#define	SHRT_MIN	(-32768)
#define	USHRT_MAX	65535
#define	INT_MAX		2147483647
#define	INT_MIN		(-2147483647-1)
#define	UINT_MAX	0xffffffff
#define	LONG_MAX	2147483647L
#define	LONG_MIN	(-2147483647L-1)
#define	ULONG_MAX	0xffffffffUL

#define	WORD_BIT	32
#define	LONG_BIT	32

#define DBL_DIG		15
#define DBL_MAX		1.7976931348623157E+308
#define DBL_MIN		2.2250738585072014E-308

#define FLT_DIG		6
#define FLT_MAX		3.40282347E+38F
#define FLT_MIN		1.17549435E-38F

#define	CLK_TCK		100
#define	ARG_MAX		5120
#define	LINK_MAX	1000
#define	PATH_MAX	256
#define	PIPE_BUF	5120
#define	PIPE_MAX	5120
#define	PASS_MAX	8
#define	CHILD_MAX	25
#define	SYS_NMLN	9
#define	UID_MAX		60000
#define	FCHR_MAX	1048576

#endif /* _SCO_LIMITS_H_ */
