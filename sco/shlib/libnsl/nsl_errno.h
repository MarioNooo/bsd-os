/*
 * Copyright (c) 1993, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI nsl_errno.h,v 2.1 1995/02/03 15:19:08 polk Exp
 */

/* from iBCS2 pp 6-32 - 6-34 */
#define	SCO_EAGAIN	11
#define	SCO_ENOMSG	35
#define	SCO_EIDRM	36
#define	SCO_ECHRNG	37
#define	SCO_EL2NSYNC	38
#define	SCO_EL3HLT	39
#define	SCO_EL3RST	40
#define	SCO_ELNRNG	41
#define	SCO_EUNATCH	42
#define	SCO_ENOCSI	43
#define	SCO_EL2HLT	44
#define	SCO_EDEADLK	45
#define	SCO_ENOLCK	46
#define	SCO_ENOSTR	60
#define	SCO_ENODATA	61
#define	SCO_ETIME	62
#define	SCO_ENOSR	63
#define	SCO_ENONET	64
#define	SCO_ENOPKG	65
#define	SCO_EREMOTE	66
#define	SCO_ENOLINK	67
#define	SCO_EADV	68
#define	SCO_ESRMNT	69
#define	SCO_ECOMM	70
#define	SCO_EPROTO	71
#define	SCO_EMULTIHOP	74
#define	SCO_ELBIN	75
#define	SCO_EDOTDOT	76
#define	SCO_EBADMSG	77
#define	SCO_ENAMETOOLONG	78
#define	SCO_EOVERFLOW	79
#define	SCO_ENOTUNIQ	80
#define	SCO_EBADFD	81
#define	SCO_EREMCHG	82

#define	SCO_EEILSEQ	88
#define	SCO_ENOSYS	89

/* XXX bogus */
#define	SCO_ELOOP	90
#define	SCO_ERESTART	91
#define	SCO_ESTRPIPE	92
#define	SCO_ENOTEMPTY	93
#define	SCO_EUSERS	94

/* XXX bogus -- not present in SCO */
/* from SVr4 API pp 13-15 */
#define	SCO_ENOTSOCK		95
#define	SCO_EDESTADDRREQ	96
#define	SCO_EMSGSIZE		97
#define	SCO_EPROTOTYPE		98
#define	SCO_ENOPROTOOPT		99
#define	SCO_EPROTONOSUPPORT	120
#define	SCO_ESOCKTNOSUPPORT	121
#define	SCO_EOPNOTSUPP		122
#define	SCO_EPFNOSUPPORT	123
#define	SCO_EAFNOSUPPORT	124
#define	SCO_EADDRINUSE		125
#define	SCO_EADDRNOTAVAIL	126
#define	SCO_ENETDOWN		127
#define	SCO_ENETUNREACH		128
#define	SCO_ENETRESET		129
#define	SCO_ECONNABORTED	130
#define	SCO_ECONNRESET		131
#define	SCO_ENOBUFS		132
#define	SCO_EISCONN		133
#define	SCO_ENOTCONN		134
#define	SCO_ENOTNAM		137
#define	SCO_ENAVAIL		138
#define	SCO_EISNAM		139
#define	SCO_ESHUTDOWN		143
#define	SCO_ETOOMANYREFS	144
#define	SCO_ETIMEDOUT		145
#define	SCO_ECONNREFUSED	146
#define	SCO_EHOSTDOWN		147
#define	SCO_EHOSTUNREACH	148
#define	SCO_EALREADY		149
#define	SCO_EINPROGRESS		150
#define	SCO_ESTALE		151
#define	SCO_ENOLOAD		152
#define	SCO_ERELOC		153
#define	SCO_ENOMATCH		154
#define	SCO_EBADVER		156
#define	SCO_ECONFIG		157

/* by observation */
#define	SCO_EWOULDBLOCK		 90

