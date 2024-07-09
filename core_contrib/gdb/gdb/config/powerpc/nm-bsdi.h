/*	BSDI nm-bsdi.h,v 1.1 2003/02/02 02:00:47 donn Exp	*/

#include "config/nm-bsdi.h"

extern int ppc_register_u_addr PARAMS ((int, int));

#define	REGISTER_U_ADDR(addr, blockend, regnum) \
	((addr) = ppc_register_u_addr((blockend), (regnum)))

#define	FETCH_INFERIOR_REGISTERS
