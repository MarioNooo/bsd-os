/*	BSDI errno.h,v 1.1 1995/07/10 18:20:54 donn Exp	*/

#ifndef _SCO_ERRNO_H_
#define	_SCO_ERRNO_H_

/* from iBCS2 pp 6-32 - 6-34 */
#define EPERM		1
#define ENOENT		2
#define ESRCH		3
#define EINTR		4
#define EIO		5
#define ENXIO		6
#define E2BIG		7
#define ENOEXEC		8
#define EBADF		9
#define ECHILD		10
#define EAGAIN		11
#define ENOMEM		12
#define EACCES		13
#define EFAULT		14
#define ENOTBLK		15
#define EBUSY		16
#define EEXIST		17
#define EXDEV		18
#define ENODEV		19
#define ENOTDIR		20
#define EISDIR		21
#define EINVAL		22
#define ENFILE		23
#define EMFILE		24
#define ENOTTY		25
#define ETXTBSY		26
#define EFBIG		27
#define ENOSPC		28
#define ESPIPE		29
#define EROFS		30
#define EMLINK		31
#define EPIPE		32
#define EDOM		33
#define ERANGE		34
#define ENOMSG		35
#define EIDRM		36
#define ECHRNG		37
#define EL2NSYNC	38
#define EL3HLT		39
#define EL3RST		40
#define ELNRNG		41
#define EUNATCH		42
#define ENOCSI		43
#define EL2HLT		44
#define EDEADLK		45
#define ENOLCK		46
#define ENOSTR		60
#define ENODATA		61
#define ETIME		62
#define ENOSR		63
#define ENONET		64
#define ENOPKG		65
#define EREMOTE		66
#define ENOLINK		67
#define EADV		68
#define ESRMNT		69
#define ECOMM		70
#define EPROTO		71
#define EMULTIHOP	74
#define ELBIN		75
#define EDOTDOT		76
#define EBADMSG		77
#define ENAMETOOLONG	78
#define EOVERFLOW	79
#define ENOTUNIQ	80
#define EBADFD		81
#define EREMCHG		82
#define EEILSEQ		88
#define ENOSYS		89
#define ELOOP		90
#define EWOULDBLOCK	90
#define ERESTART	91
#define ESTRPIPE	92
#define ENOTEMPTY	93
#define EUSERS		94

/* from SVr4 API pp 13-15 */
#define ENOTSOCK		95
#define EDESTADDRREQ		96
#define EMSGSIZE		97
#define EPROTOTYPE		98
#define ENOPROTOOPT		99
#define EPROTONOSUPPORT		120
#define ESOCKTNOSUPPORT		121
#define EOPNOTSUPP		122
#define EPFNOSUPPORT		123
#define EAFNOSUPPORT		124
#define EADDRINUSE		125
#define EADDRNOTAVAIL		126
#define ENETDOWN		127
#define ENETUNREACH		128
#define ENETRESET		129
#define ECONNABORTED		130
#define ECONNRESET		131
#define ENOBUFS			132
#define EISCONN			133
#define ENOTCONN		134
#define ENOTNAM			137
#define ENAVAIL			138
#define EISNAM			139
#define ESHUTDOWN		143
#define ETOOMANYREFS		144
#define ETIMEDOUT		145
#define ECONNREFUSED		146
#define EHOSTDOWN		147
#define EHOSTUNREACH		148
#define EALREADY		149
#define EINPROGRESS		150
#define ESTALE			151
#define ENOLOAD			152
#define ERELOC			153
#define ENOMATCH		154
#define EBADVER			156
#define ECONFIG			157

/* bizarre; iBCS2 p 6-43 */
#define	EIORESID		500

extern int errno;

#endif /* _SCO_ERRNO_H_ */
