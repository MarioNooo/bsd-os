/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI tiuser.h,v 2.1 1995/02/03 15:21:12 polk Exp
 */

/*
 * Error return values
 */
#define TBADADDR	1
#define TBADOPT		2
#define TACCES		3
#define TBADF		4
#define TNOADDR		5
#define TOUTSTATE	6
#define TBADSEQ		7
#define TSYSERR		8
#define TLOOK		9
#define TBADDATA	10
#define TBUFOVFLW	11
#define TFLOW		12
#define TNODATA		13
#define TNODIS		14
#define TNOUDERR	15
#define TBADFLAG	16
#define TNOREL		17
#define TNOTSUPPORT	18
#define TSTATECHNG	19
#define TADDRBUSY	20	/* 2DO verify this one */

/*
 * Event Bitmasks
 */
#define T_LISTEN	0x01
#define T_CONNECT	0x02	
#define T_DATA		0x04
#define T_EXDATA	0x08
#define T_DISCONNECT	0x10
#define T_ERROR		0x20
#define T_UDERR		0x40
#define T_ORDREL	0x80
#define T_EVENTS	0xff

/*
 * Flags
 */
#define T_MORE		0x01
#define T_EXPEDITED	0x02
#define T_NEGOTIATED	0x04
#define T_CHECK		0x08
#define T_DEFAULT	0x10
#define T_SUCCESS	0x20
#define T_FAILURE	0x40

/*
 * Service Types
 */
#define T_COTS		1
#define T_COTS_ORD	2
#define T_CLTS		3

struct t_info {
	long	addr;
	long	options;
	long	tsdu;
	long	etsdu;
	long	connect;
	long	discon;
	long	servtype;
};

#define T_NULL (0)
#define T_INFINITE (-1)
#define T_INVALID (-2)

struct netbuf {
	unsigned int	maxlen;
	unsigned int	len;
	char		*buf;
};

struct t_bind {
	struct netbuf	addr;
	unsigned	qlen;
};

struct t_optmgmt {
	struct netbuf	opt;
	long		flags;
};

struct t_discon {
	struct netbuf	udata;
	int		reason;
	int		sequence;
};

struct t_call {
	struct netbuf	addr;
	struct netbuf	opt;
	struct netbuf	udata;
	int		sequence;
};

struct t_unitdata {
	struct netbuf	addr;
	struct netbuf	opt;
	struct netbuf	udata;
};

struct t_uderr {
	struct netbuf	addr;
	struct netbuf	opt;
	long		error;
};

/*
 * Structure Types
 */
#define T_BIND		1
#define T_OPTMGMT	2
#define T_CALL		3
#define T_DIS		4
#define T_UNITDATA	5
#define T_UDERROR	6
#define T_INFO		7

/*
 * Fields of Structures
 */
#define T_ADDR		0x1
#define T_OPT		0x2
#define T_UDATA		0x4
#define T_ALL		0x7

/*
 * Transport Interface States
 */
#define T_UNINIT	0
#define T_UNBND		1
#define T_IDLE		2
#define T_OUTCON	3
#define T_INCON		4
#define T_DATAXFER	5
#define T_OUREL		6
#define T_INREL		7
#define	T_FAKE		8
#define T_NOSTATES	9

/*
 * User Level Events
 */
#define T_OPEN		0
#define T_BIND		1
#define T_OPTMGMT	2
#define T_UNBIND	3
#define T_CLOSE		4
#define T_SNDUDATA	5
#define T_RCVUDATA	6
#define T_RCVUDERR	7
#define T_CONNECT1	8
#define T_CONNECT2	9
#define T_RCVCONNECT	10
#define T_LISTN		11
#define T_ACCEPT1	12
#define T_ACCEPT2	13
#define T_ACCEPT3	14
#define T_SND		15
#define T_RCV		16
#define T_SNDDIS1	17
#define T_SNDDIS2	18
#define T_RCVDIS1	19
#define T_RCVDIS2	20
#define T_RCVDIS3	21
#define T_SNDREL	22
#define T_RCVREL	23
#define T_PASSCON	24
#define T_NOEVENTS	25

