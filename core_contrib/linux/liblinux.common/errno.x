/*	BSDI errno.x,v 1.1.1.1 1998/12/23 19:57:26 prb Exp	*/

/*
 * Define the errno transform.
 */

include "errno.xh"

%{

/*
 * Create an external entry point for the transform.
 */
int
__errno_out(int n)
{

	return (errno_t_out(n));
}

%}
