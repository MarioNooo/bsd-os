/*	BSDI stddef.h,v 1.1 1995/07/10 18:22:05 donn Exp	*/

#ifndef	_SCO_STDDEF_H_
#define	_SCO_STDDEF_H_

#include <machine/ansi.h>

#ifdef	_SCO_SIZE_T_
typedef _SCO_SIZE_T_ size_t;
#undef	_SCO_SIZE_T_
#endif

/* from iBCS2 p6-53 */
#define	NULL		0
#define	offsetof(s, m)	(size_t)(&(((s *)0)->m))

#endif /* _SCO_STDDEF_H_ */
