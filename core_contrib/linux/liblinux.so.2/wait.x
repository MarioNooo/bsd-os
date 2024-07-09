/*	BSDI wait.x,v 1.2 2001/03/16 15:31:14 donn Exp	*/

/*
 * Wait.h function transforms.  Amazingly, wait4() seems to be compatible
 * between Linux and BSD.
 */

include "types.xh"
include "wait.xh"
include "resource.xh"

int wait4(pid_t p, int *status, waitopt_t options, struct rusage *rup);
