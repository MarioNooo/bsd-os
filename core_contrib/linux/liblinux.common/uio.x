/*	BSDI uio.x,v 1.1.1.1 1998/12/23 19:57:26 prb Exp	*/

/*
 * Uio.h syscall transformations.
 */

include "types.xh"

%{
#include <sys/uio.h>
%}

int readv(int fd, const struct iovec *iov, int iovcnt);

int writev(int fd, const struct iovec *iov, int iovcnt);
