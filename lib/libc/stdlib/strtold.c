/*
 *	BSDI strtold.c,v 2.1 2003/06/02 21:30:35 torek Exp
 */

#include <stdlib.h>

/*
 * XXX - This implementation is obvious lacking in precision.
 * However, it will serve until we can integrate the proper code.
 */
long double
strtold(const char *str, char **endp)
{

	return (strtod(str, endp));
}
