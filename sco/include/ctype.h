/*	BSDI ctype.h,v 1.1 1995/07/10 18:20:47 donn Exp	*/

#ifndef _SCO_CTYPE_H_
#define	_SCO_CTYPE_H_

/* from iBCS2 p6-30 */
#define	_U	0x01
#define	_L	0x02
#define	_N	0x04
#define	_S	0x08
#define	_P	0x10
#define	_C	0x20
#define	_B	0x40
#define	_X	0x80

extern unsigned char _ctype[];

#define	isalpha(b)	((_ctype + 1)[b] & (_U|_L))
#define	isupper(b)	((_ctype + 1)[b] & _U)
#define	islower(b)	((_ctype + 1)[b] & _L)
#define	isdigit(b)	((_ctype + 1)[b] & _N)
#define	isxdigit(b)	((_ctype + 1)[b] & _X)
#define	isalnum(b)	((_ctype + 1)[b] & (_U|_L|_N))
#define	isspace(b)	((_ctype + 1)[b] & _S)
#define	ispunct(b)	((_ctype + 1)[b] & _P)
#define	isprint(b)	((_ctype + 1)[b] & (_U|_L|_N|_P|_B))
#define	isgraph(b)	((_ctype + 1)[b] & (_U|_L|_N|_P))
#define	iscntrl(b)	((_ctype + 1)[b] & _C)

#define	isascii(b)	(!((b) & ~0x7f))
#define	toascii(b)	((b) & 0x7f)

#define	_toupper(b)	((_ctype + 258)[b])
#define	_tolower(b)	((_ctype + 258)[b])

/* ANSI X3.159-1989 p105-106 */
#define	toupper(b)	_toupper(b)
#if 0
#define	tolower(b)	_tolower(b)
#else
extern int tolower();
#endif

#endif /* _SCO_CTYPE_H_ */
