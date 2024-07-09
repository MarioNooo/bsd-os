/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_syscall.h,v 1.5 2000/03/20 23:19:05 jch Exp 
 */

#ifndef	_THREAD_SYSCALL_H_
#define	_THREAD_SYSCALL_H_

#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <signal.h>
#include <time.h>
#include <unistd.h>

__BEGIN_DECLS
/* Threaded versions of the remapped system/library calls. */
int 	_thread_sys_accept __P((int, struct sockaddr *, int *));
int 	_thread_sys_close __P((int));
int 	_thread_sys_connect __P((int, const struct sockaddr *, int));
int 	_thread_sys_dup __P((int));
int 	_thread_sys_dup2 __P((int, int));
int 	_thread_sys_fcntl __P((int, int, int));
pid_t 	_thread_sys_fork __P((void));
int	_thread_sys_getdtablesize __P((void));
int 	_thread_sys_getitimer __P((int, struct itimerval *));
int	_thread_sys_getrlimit __P((int, struct rlimit *));
int 	_thread_sys_open __P((const char *, int, int));
int 	_thread_sys_pipe __P((int [2]));
ssize_t _thread_sys_read __P((int, void *, size_t));
ssize_t _thread_sys_readv __P((int, const struct iovec *, int));
ssize_t _thread_sys_recvfrom __P((int, void *, size_t, int, 
		struct sockaddr *, int *));
ssize_t _thread_sys_recvmsg __P((int, struct msghdr *, int));
int 	_thread_sys_select __P((int, fd_set *, fd_set *, fd_set *, 
		struct timeval *));
int 	_thread_sys_pselect __P((int, fd_set *, fd_set *, fd_set *, 
		struct timespec *, const sigset_t *));
ssize_t _thread_sys_sendmsg __P((int, const struct msghdr *, int));
ssize_t _thread_sys_send __P((int, const void *, size_t, int));
ssize_t _thread_sys_sendto __P((int, const void *, size_t, int, 
		const struct sockaddr *, int));
int 	_thread_sys_setitimer __P((int, struct itimerval *, 
		struct itimerval *));
int	_thread_sys_setrlimit __P((int, const struct rlimit *));
int 	_thread_sys_sigaction __P((int, const struct sigaction *, 
		struct sigaction *));
int 	_thread_sys_sigaltstack __P((const struct sigaltstack *, 
		struct sigaltstack *));
int 	_thread_sys_sigpending __P((sigset_t *));
int 	_thread_sys_sigprocmask __P((int, const sigset_t *, sigset_t *));
int 	_thread_sys_sigreturn __P((struct sigcontext *));
int 	_thread_sys_sigsuspend __P((const sigset_t *));
int 	_thread_sys_sigwait __P((const sigset_t *, int *));
int 	_thread_sys_socket __P((int, int, int));
int 	_thread_sys_socketpair __P((int, int, int, int [2]));
pid_t 	_thread_sys_wait4 __P((pid_t, int *, int, struct rusage *));
ssize_t _thread_sys_write __P((int, const void *, size_t));
ssize_t _thread_sys_writev __P((int, const struct iovec *, int));

/* Remapped standard system calls. */
int 	_syscall_sys_accept __P((int, struct sockaddr *, int *));
int 	_syscall_sys_close __P((int));
int 	_syscall_sys_connect __P((int, const struct sockaddr *, int));
int 	_syscall_sys_dup __P((int));
int 	_syscall_sys_dup2 __P((int, int));
int	_syscall_sys_execve __P((const char *, char *const *, char *const *));
int 	_syscall_sys_fcntl __P((int, int, int));
pid_t 	_syscall_sys_fork __P((void));
int	_syscall_sys_getdtablesize __P((void));
int 	_syscall_sys_getitimer __P((int, struct itimerval *));
int	_syscall_sys_getrlimit __P((int, struct rlimit *));
int 	_syscall_sys_open __P((const char *, int, int));
int 	_syscall_sys_pipe __P((int [2]));
ssize_t _syscall_sys_read __P((int, void *, size_t));
ssize_t _syscall_sys_readv __P((int, const struct iovec *, int));
ssize_t _syscall_sys_recvfrom __P((int, void *, size_t, int, struct sockaddr *, int *));
ssize_t _syscall_sys_recvmsg __P((int, struct msghdr *, int));
int 	_syscall_sys_select __P((int, fd_set *, fd_set *, fd_set *, 
		struct timeval *));
ssize_t _syscall_sys_send __P((int, const void *, size_t, int));
ssize_t _syscall_sys_sendmsg __P((int, const struct msghdr *, int));
ssize_t _syscall_sys_sendto __P((int, const void *, size_t, int, 
		const struct sockaddr *, int));
int 	_syscall_sys_setitimer __P((int, struct itimerval *, 
		struct itimerval *));
int	_syscall_sys_setrlimit __P((int, const struct rlimit *));
int 	_syscall_sys_sigaction __P((int, const struct sigaction *, 
		struct sigaction *));
int 	_syscall_sys_sigaltstack __P((const struct sigaltstack *, 
		struct sigaltstack *));
int 	_syscall_sys_sigpending __P((sigset_t *));
int 	_syscall_sys_sigprocmask __P((int, const sigset_t *, sigset_t *));
int 	_syscall_sys_sigreturn __P((struct sigcontext *));
int 	_syscall_sys_sigsuspend __P((const sigset_t *));
int 	_syscall_sys_sigwait __P((const sigset_t *, int *));
int 	_syscall_sys_socket __P((int, int, int));
int 	_syscall_sys_socketpair __P((int, int, int, int [2]));
pid_t 	_syscall_sys_wait4 __P((pid_t, int *, int, struct rusage *));
ssize_t _syscall_sys_write __P((int, const void *, size_t));
ssize_t _syscall_sys_writev __P((int, const struct iovec *, int));
__END_DECLS

#endif
