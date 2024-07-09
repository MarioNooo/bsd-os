/*	BSDI stropts.h,v 1.1 1995/07/10 18:26:18 donn Exp	*/

#ifndef _SCO_SYS_STROPTS_H_
#define	_SCO_SYS_STROPTS_H_

/* from iBCS2 p6-56 - 6-58 */

#define	RNORM		0
#define	RMSGD		1
#define	RMSGN		2

#define	FLUSHR		1
#define	FLUSHW		2
#define	FLUSHRW		3

#define	S_INPUT		0x01
#define	S_HIPRI		0x02
#define	S_OUTPUT	0x04
#define	S_MSG		0x08

#define	RS_HIPRI	1

#define	MORECTL		1
#define	MOREDATA	2

#define	__S		('S' << 8)
#define	I_NREAD		(__S|1)
#define	I_PUSH		(__S|2)
#define	I_POP		(__S|3)
#define	I_LOOK		(__S|4)
#define	I_FLUSH		(__S|5)
#define	I_SRDOPT	(__S|6)
#define	I_GRDOPT	(__S|7)
#define	I_STR		(__S|8)
#define	I_SETSIG	(__S|9)
#define	I_GETSIG	(__S|10)
#define	I_FIND		(__S|11)
#define	I_LINK		(__S|12)
#define	I_UNLINK	(__S|13)
#define	I_RECVFD	(__S|14)
#define	I_PEEK		(__S|15)
#define	I_FDINSERT	(__S|16)
#define	I_SENDFD	(__S|17)

struct strioctl {
	int		ic_cmd;
	int		ic_timout;
	int		ic_len;
	char		*ic_dp;
};

struct strbuf {
	int		maxlen;
	int		len;
	char		*buf;
};

struct strpeek {
	struct strbuf	ctlbuf;
	struct strbuf	databuf;
	long		flags;
};

struct strfdinsert {
	struct strbuf	ctlbuf;
	struct strbuf	databuf;
	long		flags;
	int		fildes;
	int		offset;
};

struct strrecvfd {
	int		fd;
	unsigned short	uid;
	unsigned short	gid;
	char		fill[8];
};

#endif	/* _SCO_SYS_STROPTS_H_ */
