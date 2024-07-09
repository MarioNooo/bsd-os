/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_recvmsg.c,v 1.5 2003/09/04 20:10:44 jch Exp
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"
#include "thread_trace.h"

ssize_t
_thread_sys_recvmsg(fd, msg, flags)
	int fd;
	struct msghdr *msg;
	int flags;
{
	struct cmsghdr *cmsg;
	int *fdp, *lp, ret;

	TR("_thread_sys_recvmsg", fd, msg);

	errno = 0;
	_thread_kern_enter();

	THREAD_AIO(fd, FD_READ, ret,
	    _syscall_sys_recvmsg(fd, msg, flags),
	    goto Return);

	/* Scan for file descriptors passed on AF_LOCAL sockets */
	cmsg = NULL;
	while ((cmsg = CMSG_NXTHDR(msg, cmsg)) != NULL) {
		if (cmsg->cmsg_level != SOL_SOCKET ||
		    cmsg->cmsg_type != SCM_RIGHTS)
			continue;
		lp = (int *)((char *)cmsg + cmsg->cmsg_len);
		for (fdp = (int *)(cmsg + 1); fdp < lp; fdp++)
			if (*fdp >= 0 &&
			    _thread_aio_entry_init(*fdp) != 0)
				abort();
	}

 Return:
	_thread_kern_exit();
	return (ret);
}
