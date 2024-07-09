/*	BSDI param.h,v 1.1 1995/07/10 18:25:41 donn Exp	*/

#ifndef _SCO_SYS_PARAM_H_
#define	_SCO_SYS_PARAM_H_

#include <sys/types.h>
#include <sys/bsdtypes.h>

/* from iBCS2 p6-44 */
#define	CANBSIZ		256
#define	DEV_BSIZE	512
#define	HZ		100
#define	MAXFRAG		8
#define	MAXLINK		1000
#define	MAXNAMELEN	256
#define	MAXPATHLEN	1024
#define	MAXPID		30000
#define	MAXSYMLINKS	20
#define	MAXUID		60000
#define	NADDR		13
#define	NBBY		8
#define	NBPSCTR		512
#define	NCARGS		5120
#define	NOFILES_MAX	100
#define	NOFILES_MIN	20
#define	PIPE_MAX	5120
#define	TICK		10000000

#endif	/* _SCO_SYS_PARAM_H_ */
