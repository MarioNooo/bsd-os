/*	BSDI	socket.x,v 1.7 2001/01/22 16:49:28 donn Exp	*/

/*
 * Transforms for socket-related syscalls.
 */

include "types.xh"
include "socket.xh"
include "socketcall.xh"

%{
#define	SOK_UN_LEN	106

/* Convert from library errno protocol back to syscall protocol (sigh).  */
static inline _K(int r) { return (r == -1 ? -errno : r); }
%}

int __library_accept(int fd, struct sockaddr *sa, socklen_t *length_sa);

int __library_bind(int fd, const struct sockaddr *sa, socklen_t length_sa)
{
	if (length_sa > SOK_UN_LEN)
		length_sa = SOK_UN_LEN;
	return (__bsdi_syscall(SYS_bind, fd, sa, length_sa));
}

int __library_connect(int fd, const struct sockaddr *sa, socklen_t length_sa)
{
	if (length_sa > SOK_UN_LEN)
		length_sa = SOK_UN_LEN;
	return (__bsdi_syscall(SYS_connect, fd, sa, length_sa));
}

int __library_getpeername(int fd, struct sockaddr *sa, socklen_t *length_sa);

int __library_getsockname(int fd, struct sockaddr *sa, socklen_t *length_sa);

int __library_getsockopt(int fd, socklevel_t level, sockopt_t optname,
    struct sockopt *optval, size_t *length_optval);

int __library_listen(int fd, int backlog);

ssize_t __library_recvfrom(int fd, void *buf, size_t len, recvflags_t flags,
    struct sockaddr *from, socklen_t *length_from);

/* XXX We don't bother translating msghdr flags... */
int __library_recvmsg(int fd, struct msghdr *msg, recvflags_t flags);

int __library_sendto(int fd, const struct msghdr *msg, size_t len, int flags,
    const struct sockaddr *to, socklen_t length_to);

int __library_sendmsg(int fd, const struct msghdr *msg, recvflags_t flags);

int __library_setsockopt(int fd, socklevel_t level, sockopt_t optname,
    const struct sockopt *optval, size_t length_optval);

int __library_shutdown(int fd, int how);

int __library_socket(familycookie_t domain, int type, familycookie_t protocol);

int __library_socketpair(familycookie_t d, int type, familycookie_t protocol,
    int *sv);

ssize_t __library_recv(int fd, struct msghdr *msg, size_t len,
    recvflags_t flags)
{
	return (__bsdi_syscall(SYS_recvfrom, fd, msg, len, flags, NULL, 0));
}

ssize_t __library_send(int fd, const struct msghdr *msg, size_t len,
    recvflags_t flags)
{
	return (__bsdi_syscall(SYS_sendto, fd, msg, len, flags, NULL, 0));
}

/*
 * Implement the (inline-only) multiplexed kernel socket call in Linux.
 * We just call the emulated functions, which apply their own transformations.
 */
noerrno int
__kernel_socketcall(SOCKOP_socket, const struct linux_socket_parms *p) {
	static int linux_socket(unsigned short, int, unsigned short);
	return (_K(linux_socket(p->domain, p->type, p->protocol)));
}

noerrno int
__kernel_socketcall(SOCKOP_bind, const struct linux_bind_parms *p) {
	static int linux_bind(int, const struct linux_sockaddr *,
	    linux_socklen_t);
	return (_K(linux_bind(p->fd, p->sa, p->length_sa)));
}

noerrno int
__kernel_socketcall(SOCKOP_connect, const struct linux_connect_parms *p) {
	static int linux_connect(int, const struct linux_sockaddr *,
	    linux_socklen_t);
	return (_K(linux_connect(p->fd, p->sa, p->length_sa)));
}

noerrno int
__kernel_socketcall(SOCKOP_listen, const struct linux_listen_parms *p) {
	static int linux_listen(int, int);
	return (_K(linux_listen(p->fd, p->backlog)));
}

noerrno int
__kernel_socketcall(SOCKOP_accept, const struct linux_accept_parms *p) {
	static int linux_accept(int, struct linux_sockaddr *,
	    linux_socklen_t *);
	return (_K(linux_accept(p->fd, p->sa, p->length_sa)));
}

noerrno int
__kernel_socketcall(SOCKOP_getsockname,
    const struct linux_getsockname_parms *p) {
	static int linux_getsockname(int, struct linux_sockaddr *,
	    linux_socklen_t *);
	return (_K(linux_getsockname(p->fd, p->sa, p->length_sa)));
}

noerrno int
__kernel_socketcall(SOCKOP_getpeername,
    const struct linux_getpeername_parms *p) {
	static int linux_getpeername(int, struct linux_sockaddr *,
	    linux_socklen_t *);
	return (_K(linux_getpeername(p->fd, p->sa, p->length_sa)));
}

noerrno int
__kernel_socketcall(SOCKOP_socketpair, const struct linux_socketpair_parms *p) {
	static int linux_socketpair(unsigned short, int, unsigned short, int *);
	return (_K(linux_socketpair(p->d, p->type, p->protocol, p->sv)));
}

noerrno int
__kernel_socketcall(SOCKOP_send, const struct linux_send_parms *p) {
	static int linux_send(int, const struct msghdr *, size_t, int);
	return (_K(linux_send(p->fd, p->msg, p->len, p->flags)));
}

noerrno int
__kernel_socketcall(SOCKOP_recv, const struct linux_recv_parms *p) {
	static int linux_recv(int, struct msghdr *, size_t, int);
	return (_K(linux_recv(p->fd, p->msg, p->len, p->flags)));
}

noerrno int
__kernel_socketcall(SOCKOP_sendto, const struct linux_sendto_parms *p) {
	static int linux_sendto(int, const struct msghdr *, size_t, int,
	    const struct linux_sockaddr *, linux_socklen_t);
	return (_K(linux_sendto(p->fd, p->msg, p->len, p->flags, p->to,
	    p->length_to)));
}

noerrno int
__kernel_socketcall(SOCKOP_recvfrom, const struct linux_recvfrom_parms *p) {
	static int linux_recvfrom(int, void *, size_t, int,
	    struct linux_sockaddr *, linux_socklen_t *);
	return (_K(linux_recvfrom(p->fd, p->buf, p->len, p->flags, p->from,
	    p->length_from)));
}

noerrno int
__kernel_socketcall(SOCKOP_shutdown, const struct linux_shutdown_parms *p) {
	static int linux_shutdown(int, int);
	return (_K(linux_shutdown(p->fd, p->how)));
}

noerrno int
__kernel_socketcall(SOCKOP_setsockopt, const struct linux_setsockopt_parms *p) {
	static int linux_setsockopt(int, int, int, const struct linux_sockopt *,
	    size_t);
	return (_K(linux_setsockopt(p->fd, p->level, p->optname, p->optval,
	    p->length_optval)));
}

noerrno int
__kernel_socketcall(SOCKOP_getsockopt, const struct linux_getsockopt_parms *p) {
	static int linux_getsockopt(int, int, int, struct linux_sockopt *,
	    size_t *);
	return (_K(linux_getsockopt(p->fd, p->level, p->optname, p->optval,
	    p->length_optval)));
}

noerrno int
__kernel_socketcall(SOCKOP_sendmsg, const struct linux_sendmsg_parms *p) {
	static int linux_sendmsg(int, const struct msghdr *, int);
	return (_K(linux_sendmsg(p->fd, p->msg, p->flags)));
}

noerrno int
__kernel_socketcall(SOCKOP_recvmsg, const struct linux_recvmsg_parms *p) {
	static int linux_recvmsg(int, struct msghdr *, int);
	return (_K(linux_recvmsg(p->fd, p->msg, p->flags)));
}

int __kernel_socketcall(linux_socketcall_t sc, unsigned long *p) = ENOSYS;
