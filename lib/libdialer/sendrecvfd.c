#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>
#include <libdialer.h>
#include <string.h>

#define	MAXFDS	64	/* Can pass up to 64 fds per message */

int
recvfd(int soket)
{
	int fd;
	int cnt = 1;

	return(recvfds(soket, &fd, &cnt));
}

int
sendfd(int soket, int fd)
{
	return(sendfds(soket, &fd, 1));
}

int
recvfds(int soket, int *fds, int *cnt)
{
        u_char _cmsg[sizeof(struct cmsghdr) + sizeof(int) * MAXFDS];
        struct cmsghdr *cmsg = (struct cmsghdr *)_cmsg;
        struct msghdr msg;
	int rcnt;

	if (*cnt < 1 || *cnt > MAXFDS) {
		errno = EINVAL;
		return(-1);
	}

        memset(_cmsg, 0, sizeof(_cmsg));
        memset(&msg, 0, sizeof(msg));
        msg.msg_control = (caddr_t)cmsg;  
        msg.msg_controllen = sizeof(_cmsg);

        if (recvmsg(soket, &msg, 0) < 0)
                return(-1);
        if (cmsg->cmsg_len - sizeof(struct cmsghdr) < sizeof(int) ||
            cmsg->cmsg_type != SCM_RIGHTS) {
		errno = EFTYPE;
                return(-1); 
        }
	rcnt = (cmsg->cmsg_len - sizeof(struct cmsghdr)) / sizeof(int);
	if (rcnt < *cnt)
		memcpy(fds, CMSG_DATA(cmsg), rcnt * sizeof(int));
	else
		memcpy(fds, CMSG_DATA(cmsg), *cnt * sizeof(int));
	*cnt = rcnt;
        return(fds[0]);
}

int
sendfds(int soket, int *fds, int cnt)
{
    	u_char _cmsg[sizeof(struct cmsghdr) + sizeof(int) * MAXFDS];
    	struct cmsghdr *cmsg = (struct cmsghdr *)_cmsg;
	struct msghdr msg;

	if (cnt < 1 || cnt > MAXFDS) {
		errno = EINVAL;
		return(-1);
	}

	cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(int) * cnt;
    	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	memcpy(CMSG_DATA(cmsg), fds, sizeof(int) * cnt);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = (caddr_t)cmsg;
	msg.msg_controllen = cmsg->cmsg_len;

	return(sendmsg(soket, &msg, 0));
}
