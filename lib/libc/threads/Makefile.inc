#	Makefile.inc,v 1.21 2001/05/08 16:53:26 torek Exp

# thread sources
CFLAGS+=-I${.CURDIR}/threads
.PATH:	${.CURDIR}/threads ${.CURDIR}/${MACHINE}/threads

IO_SRCS=thread_aio.c thread_accept.c thread_close.c thread_connect.c \
	thread_dup.c thread_dup2.c thread_fcntl.c thread_fsync.c \
	thread_getdtablesize.c thread_getprlimit.c thread_getrlimit.c \
	thread_ioctl.c thread_msync.c thread_open.c thread_pipe.c \
	thread_read.c thread_readv.c thread_recvfrom.c thread_recvmsg.c \
	thread_sendto.c thread_sendmsg.c thread_setprlimit.c \
	thread_setrlimit.c thread_socket.c thread_socketpair.c \
	thread_write.c thread_writev.c thread_yield.c

SRCS+=  thread_atfork.c thread_attr.c thread_attr_default.c \
	thread_attr_detachstate.c thread_attr_inheritsched.c \
	thread_attr_policy.c thread_attr_param.c thread_attr_scope.c \
	thread_attr_size.c thread_attr_addr.c thread_attr_suspstate.c \
	thread_cancel.c thread_clean.c thread_cond.c thread_create.c \
	thread_detach.c thread_equal.c thread_execve.c thread_exit.c \
	thread_fork.c thread_getstack.c thread_init.c thread_itimer.c \
	thread_join.c thread_kern.c thread_mutex.c thread_nanosleep.c \
	thread_once.c thread_qlock.c thread_queue.c thread_resume.c \
	thread_schedparam.c \
	thread_self.c thread_select.c thread_seterrno.c thread_sig.c \
	thread_sigaction.c thread_sigaltstack.c thread_sigkill.c \
	thread_sigmask.c thread_sigpending.c thread_sigreturn.c \
	thread_sigsuspend.c thread_spec.c thread_suspend.c thread_timer.c \
	thread_vfork.c thread_wait4.c \
	${IO_SRCS}

.if defined(DEBUG)
CFLAGS+= -DDEBUG -g -DKTR -I/usr/src/include
SRCS+=	thread_trace.c
.endif

MAN3+=	pthreads.0 pthread_atfork.0 pthread_attr.0 pthread_attr_detachstate.0 \
	pthread_attr_inheritsched.0 pthread_attr_schedpolicy.0 \
	pthread_attr_schedparam.0 pthread_cancel.0 pthread_cleanup.0 \
	pthread_cond.0 	pthread_create.0 pthread_detach.0 \
	pthread_equal.0 pthread_exit.0 pthread_join.0 pthread_key.0 \
	pthread_kill.0 pthread_mutex.0 pthread_once.0 pthread_schedparam.0 \
	pthread_self.0 pthread_sigmask.0 pthread_specific.0 \
	pthread_testcancel.0 sched_yield.0 pthread_attr_scope.0 \
	pthread_attr_stackaddr.0 pthread_attr_stacksize.0 pthread_np.0

MLINKS+=pthread_attr.3 pthread_attr_init.3
MLINKS+=pthread_attr.3 pthread_attr_destroy.3
MLINKS+=pthread_attr_detachstate.3 pthread_attr_getdetachstate.3
MLINKS+=pthread_attr_detachstate.3 pthread_attr_setdetachstate.3
MLINKS+=pthread_attr_inheritsched.3 pthread_attr_getinheritsched.3
MLINKS+=pthread_attr_inheritsched.3 pthread_attr_setinheritsched.3
MLINKS+=pthread_attr_schedparam.3 pthread_attr_getschedparam.3
MLINKS+=pthread_attr_schedparam.3 pthread_attr_setschedparam.3
MLINKS+=pthread_attr_schedpolicy.3 pthread_attr_getschedpolicy.3
MLINKS+=pthread_attr_schedpolicy.3 pthread_attr_setschedpolicy.3
MLINKS+=pthread_testcancel.3 pthread_setcancelstate.3
MLINKS+=pthread_testcancel.3 pthread_setcanceltype.3
MLINKS+=pthread_cleanup.3 pthread_cleanup_push.3
MLINKS+=pthread_cleanup.3 pthread_cleanup_pop.3
MLINKS+=pthread_cond.3 pthread_cond_init.3
MLINKS+=pthread_cond.3 pthread_cond_destroy.3
MLINKS+=pthread_cond.3 pthread_cond_wait.3
MLINKS+=pthread_cond.3 pthread_cond_timedwait.3
MLINKS+=pthread_cond.3 pthread_cond_signal.3
MLINKS+=pthread_cond.3 pthread_cond_broadcast.3
MLINKS+=pthread_key.3 pthread_key_create.3
MLINKS+=pthread_key.3 pthread_key_delete.3
MLINKS+=pthread_mutex.3 pthread_mutex_init.3
MLINKS+=pthread_mutex.3 pthread_mutex_destroy.3
MLINKS+=pthread_mutex.3 pthread_mutex_lock.3
MLINKS+=pthread_mutex.3 pthread_mutex_unlock.3
MLINKS+=pthread_mutex.3 pthread_mutex_trylock.3
MLINKS+=pthread_schedparam.3 pthread_setschedparam.3
MLINKS+=pthread_schedparam.3 pthread_getschedparam.3
MLINKS+=pthread_specific.3 pthread_setspecific.3
MLINKS+=pthread_specific.3 pthread_getspecific.3
MLINKS+=pthread_attr_scope.3 pthread_attr_getscope.3
MLINKS+=pthread_attr_scope.3 pthread_attr_setscope.3
MLINKS+=pthread_attr_stackaddr.3 pthread_attr_setstackaddr.3
MLINKS+=pthread_attr_stackaddr.3 pthread_attr_getstackaddr.3
MLINKS+=pthread_attr_stacksize.3 pthread_attr_setstacksize.3
MLINKS+=pthread_attr_stacksize.3 pthread_attr_getstacksize.3
MLINKS+=pthread_np.3 pthread_attr_setsuspendstate_np.3
MLINKS+=pthread_np.3 pthread_attr_getsuspendstate_np.3
MLINKS+=pthread_np.3 pthread_getstackbase_np.3
MLINKS+=pthread_np.3 pthread_getstackpointer_np.3
MLINKS+=pthread_np.3 pthread_getstacksize_np.3
MLINKS+=pthread_np.3 pthread_resume_np.3
MLINKS+=pthread_np.3 pthread_resume_all_np.3
MLINKS+=pthread_np.3 pthread_suspend_np.3
MLINKS+=pthread_np.3 pthread_suspend_all_np.3

.include "${.CURDIR}/${MACHINE}/threads/Makefile.inc"
