/*	BSDI utsname.h,v 1.1 1995/07/10 18:26:57 donn Exp	*/

#ifndef _SCO_SYS_UTSNAME_H_
#define	_SCO_SYS_UTSNAME_H_

/* from iBCS2 p6-83 */

struct utsname {
	char	sysname[9];
	char	nodename[9];
	char	release[9];
	char	version[9];
	char	machine[9];
};

#endif	/* _SCO_SYS_UTSNAME_H_ */
