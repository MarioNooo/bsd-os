/* BSDI malloc.h,v 2.1 1996/05/07 04:44:32 torek Exp */

#ifndef _MALLOC_H_
#define	_MALLOC_H_

#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#include <sys/cdefs.h>

/*
 * This header file provides compatibility for applications written using
 * historic System V header files.  The "real" function prototypes for
 * these functions are in <stdlib.h>, and new applications should use them,
 * not these.  Do not reference this include file in any manual pages.
 */
__BEGIN_DECLS
void	*calloc __P((size_t, size_t));
void	*malloc __P((size_t));
void	*realloc __P((void *, size_t));
void	 free __P((void *));
__END_DECLS

#endif /* !_MALLOC_H_ */
