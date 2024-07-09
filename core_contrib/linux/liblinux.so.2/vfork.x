/*	BSDI vfork.x,v 1.1.1.1 1998/12/23 19:57:24 prb Exp	*/

/*
 * Vfork() is a separate function only under glibc.
 */

include "types.xh"

%{
pid_t __bsdi_vfork(void);
%}

pid_t vfork() { return (__bsdi_vfork()); }
