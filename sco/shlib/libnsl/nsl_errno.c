/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI nsl_errno.c,v 2.1 1995/02/03 15:19:04 polk Exp
 */

 /*
  * We should probably put ifdefs in the emulator version of this
  * file and get rid of this one
  */

#include <sys/param.h>
#include <errno.h>
#include "nsl_errno.h"
#include "nsl_compat.h"

/*
 * Errno translation table.
 */

const int errno_out_map[] = {
	0,
	EPERM,
	ENOENT,
	ESRCH,
	EINTR,
	EIO,
	ENXIO,
	E2BIG,
	ENOEXEC,
	EBADF,
	ECHILD,
	SCO_EDEADLK,
	ENOMEM,
	EACCES,
	EFAULT,
	ENOTBLK,
	EBUSY,
	EEXIST,
	EXDEV,
	ENODEV,
	ENOTDIR,
	EISDIR,
	EINVAL,
	ENFILE,
	EMFILE,
	ENOTTY,
	ETXTBSY,
	EFBIG,
	ENOSPC,
	ESPIPE,
	EROFS,
	EMLINK,
	EPIPE,
	EDOM,
	ERANGE,
	SCO_EAGAIN,
	SCO_EINPROGRESS,
	SCO_EALREADY,
	SCO_ENOTSOCK,
	SCO_EDESTADDRREQ,
	SCO_EMSGSIZE,
	SCO_EPROTOTYPE,
	SCO_ENOPROTOOPT,
	SCO_EPROTONOSUPPORT,
	SCO_ESOCKTNOSUPPORT,
	SCO_EOPNOTSUPP,
	SCO_EPFNOSUPPORT,
	SCO_EAFNOSUPPORT,
	SCO_EADDRINUSE,
	SCO_EADDRNOTAVAIL,
	SCO_ENETDOWN,
	SCO_ENETUNREACH,
	SCO_ENETRESET,
	SCO_ECONNABORTED,
	SCO_ECONNRESET,
	SCO_ENOBUFS,
	SCO_EISCONN,
	SCO_ENOTCONN,
	SCO_ESHUTDOWN,
	SCO_ETOOMANYREFS,
	SCO_ETIMEDOUT,
	SCO_ECONNREFUSED,
	SCO_ELOOP,
	SCO_ENAMETOOLONG,
	SCO_EHOSTDOWN,
	SCO_EHOSTUNREACH,
	SCO_ENOTEMPTY,
	SCO_EAGAIN,		/* EPROCLIM -- see fork(2) in SVr4 API */
	SCO_EUSERS,
	EFBIG,			/* EDQUOT, best approximation */
	SCO_ESTALE,
	SCO_EREMOTE,		/* XXX not used? */
	SCO_EPROTO,		/* EBADRPC, best approximation */
	SCO_EPROTO,		/* ERPCMISMATCH, best approximation */
	SCO_EPROTO,		/* EPROGUNAVAIL, best approximation */
	SCO_EPROTO,		/* EPROGMISMATCH, best approximation */
	SCO_EPROTO,		/* EPROCUNAVAIL, best approximation */
	SCO_ENOLCK,
	SCO_ENOSYS,
	EINVAL,			/* EFTYPE, best approximation */
};


void
tli_maperrno() {
	if (sco__errno)
		*sco__errno =  errno_out_map[errno];
	else
		*ibc__errno =  errno_out_map[errno];
}
