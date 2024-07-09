/*	BSDI ioctl.h,v 1.1 1995/07/10 18:25:33 donn Exp	*/

#ifndef _SCO_SYS_IOCTL_H_
#define	_SCO_SYS_IOCTL_H_

/* by observation of running programs */
#define SOCK_IOCTL		0x801c4942
#define SIOCGIFFLAGS		0xc0204910
#define SIOCGIFCONF		0xc0084911
#define SIOCGIFBRDADDR		0xc0204920
#define FIONREAD		0x40045308
#define FIONBIO			0x80045309

#include <sys/cdefs.h>

int ioctl __P((int, int, int));

#endif	/* _SCO_SYS_IOCTL_H_ */
