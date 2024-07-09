/*
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_ipc.c,v 2.7 1996/12/21 16:15:19 donn Exp
 */

/*
 * Common code for emulation of iBCS2 IPC routines.
 */

#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>

#include "emulate.h"
#include "sco.h"

/*
 * iBCS2 IPC definitions for message queue, semaphore and shared memory ops
 */

/* from iBCS2 p6-39 */

struct sco_ipc_perm {
	unsigned short	uid;
	unsigned short	gid;
	unsigned short	cuid;
	unsigned short	cgid;
	unsigned short	mode;
	unsigned short	seq;
	long		key;
};

void
ipc_perm_in(sip, ip)
	const struct sco_ipc_perm *sip;
	struct ipc_perm *ip;
{

	ip->uid = sip->uid;
	ip->gid = sip->gid;
	ip->cuid = sip->cuid;
	ip->cgid = sip->cgid;
	ip->mode = sip->mode;
	ip->seq = sip->seq;
	ip->key = sip->key;
}

void
ipc_perm_out(ip, sip)
	const struct ipc_perm *ip;
	struct sco_ipc_perm *sip;
{

	sip->uid = ip->uid;
	sip->gid = ip->gid;
	sip->cuid = ip->cuid;
	sip->cgid = ip->cgid;
	sip->mode = ip->mode;
	sip->seq = ip->seq;
	sip->key = ip->key;
}

/* iBCS2 p 6-48 */

struct sco_shmid_ds {
	struct sco_ipc_perm
			shm_perm;
	int		shm_segsz;
	void		*shm_reg;
	char		pad[4];
	unsigned short	shm_lpid;
	unsigned short	shm_cpid;
	unsigned short	shm_nattch;
	unsigned short	shm_cnattch;
	long		shm_atime;
	long		shm_dtime;
	long		shm_ctime;
};

#define	SCO_SHMLBA	0x00400000

/* iBCS2 p 3-36 */
#define	SCO_SHMAT	0
#define	SCO_SHMCTL	1
#define	SCO_SHMDT	2
#define	SCO_SHMGET	3

void
shmid_ds_in(sdp, dp)
	const struct sco_shmid_ds *sdp;
	struct shmid_ds *dp;
{

	ipc_perm_in(&sdp->shm_perm, &dp->shm_perm);
	dp->shm_segsz = sdp->shm_segsz;
	dp->shm_internal = sdp->shm_reg;
	dp->shm_lpid = sdp->shm_lpid;
	dp->shm_cpid = sdp->shm_cpid;
	dp->shm_nattch = sdp->shm_nattch;
	dp->shm_atime = sdp->shm_atime;
	dp->shm_dtime = sdp->shm_dtime;
	dp->shm_ctime = sdp->shm_ctime;
}

void
shmid_ds_out(dp, sdp)
	const struct shmid_ds *dp;
	struct sco_shmid_ds *sdp;
{

	ipc_perm_out(&dp->shm_perm, &sdp->shm_perm);
	sdp->shm_segsz = dp->shm_segsz;
	sdp->shm_reg = dp->shm_internal;
	sdp->shm_lpid = dp->shm_lpid;
	sdp->shm_cpid = dp->shm_cpid;
	sdp->shm_nattch = dp->shm_nattch;
	sdp->shm_cnattch = dp->shm_nattch;	/* XXX */
	sdp->shm_atime = dp->shm_atime;
	sdp->shm_dtime = dp->shm_dtime;
	sdp->shm_ctime = dp->shm_ctime;
}

int
sco_shmat(shmid, addr, flags)
	int shmid;
	void *addr;
	int flags;
{
	unsigned long va;

	va = (unsigned long)addr;
	if (va && flags & SHM_RND)
		va &= ~(SCO_SHMLBA - 1);
	addr = (void *)va;

	return ((int)shmat(shmid, addr, flags));
}

int
sco_shmctl(shmid, cookie, sdp)
	int shmid;
	int cookie;
	struct sco_shmid_ds *sdp;
{
	struct shmid_ds d;
	int r;

	if (cookie == IPC_SET)
		shmid_ds_in(sdp, &d);
	r = shmctl(shmid, cookie, &d);
	if (r != -1 && cookie == IPC_STAT)
		shmid_ds_out(&d, sdp);
	return (r);
}

int
sco_shm(cookie, a, b, c)
	int cookie, a, b, c;
{

	switch (cookie) {
	case SCO_SHMAT:
		return (sco_shmat(a, (void *)b, c));
	case SCO_SHMCTL:
		return (sco_shmctl(a, b, (void *)c));
	case SCO_SHMDT:
		return (shmdt((void *)a));
	case SCO_SHMGET:
		return (shmget((key_t)a, b, c));
	}

	errno = EINVAL;
	return (-1);
}

/*
 * iBCS2 semaphore definitions
 */

/* from iBCS2 p 6-47 */

/* XXX This structure is 'probably' opaque, so we don't convert it. */
struct sco_sem {
	unsigned short	semval;
	short		sempid;
	unsigned short	semncnt;
	unsigned short	semzcnt;
};

struct sco_semid_ds {
	struct sco_ipc_perm
			sem_perm;
	struct sco_sem	*sem_base;
	unsigned short	sem_nsems;
	long		sem_otime;
	long		sem_ctime;
};

union sco_semun {
	int		val;
	struct sco_semid_ds
			*buf;
	unsigned short	*array;
};

/* iBCS2 p 3-35 */
#define	SCO_SEMCTL	0
#define	SCO_SEMGET	1
#define	SCO_SEMOP	2

void
semid_ds_in(sdp, dp)
	const struct sco_semid_ds *sdp;
	struct semid_ds *dp;
{

	ipc_perm_in(&sdp->sem_perm, &dp->sem_perm);
	dp->sem_base = (void *)sdp->sem_base;		/* XXX */
	dp->sem_nsems = sdp->sem_nsems;
	dp->sem_otime = sdp->sem_otime;
	dp->sem_ctime = sdp->sem_ctime;
}

void
semid_ds_out(dp, sdp)
	const struct semid_ds *dp;
	struct sco_semid_ds *sdp;
{

	ipc_perm_out(&dp->sem_perm, &sdp->sem_perm);
	sdp->sem_base = (void *)dp->sem_base;		/* XXX */
	sdp->sem_nsems = dp->sem_nsems;
	sdp->sem_otime = dp->sem_otime;
	sdp->sem_ctime = dp->sem_ctime;
}

int
sco_semctl(semid, semnum, cookie, x)
	int semid;
	int cookie;
	int semnum;
	int x;
{
	struct sco_semid_ds *sdp;
	struct semid_ds d;
	union sco_semun ssemun;
	union semun semun;
	int r;

	ssemun.val = x;
	sdp = ssemun.buf;

	switch (cookie) {
	case IPC_SET:
		semid_ds_in(sdp, &d);
		/* FALL THROUGH */
	case IPC_STAT:
		semun.buf = &d;
		break;
	case GETALL:
	case SETALL:
		semun.array = ssemun.array;
		break;
	case SETVAL:
		semun.val = ssemun.val;
		break;
	default:
		semun.val = 0;
		break;
	}

	r = semctl(semid, semnum, cookie, semun);
	if (r != -1 && cookie == IPC_STAT)
		semid_ds_out(&d, sdp);
	return (r);
}

int
sco_sem(cookie, a, b, c, d)
	int cookie, a, b, c, d;
{

	switch (cookie) {
	case SCO_SEMCTL:
		return (sco_semctl(a, b, c, d));
	case SCO_SEMGET:
		return (semget((long)a, b, c));
	case SCO_SEMOP:
		return (commit_semop(a, (struct sembuf *)b, (unsigned int)c));
	}

	errno = EINVAL;
	return (-1);
}

/* from iBCS2, p 6-43 */
struct sco_msqid_ds {
	struct sco_ipc_perm
			msg_perm;
	struct msg	*msg_first;
	struct msg	*msg_last;
	unsigned short	msg_cbytes;
	unsigned short	msg_qnum;
	unsigned short	msg_qbytes;
	unsigned short	msg_lspid;
	unsigned short	msg_lrpid;
	long		msg_stime;
	long		msg_rtime;
	long		msg_ctime;
};

/* iBCS2 p 3-35 */
#define	SCO_MSGGET	0
#define	SCO_MSGCTL	1
#define	SCO_MSGRCV	2
#define	SCO_MSGSND	3

void
msqid_ds_in(sdp, dp)
	const struct sco_msqid_ds *sdp;
	struct msqid_ds *dp;
{

	ipc_perm_in(&sdp->msg_perm, &dp->msg_perm);
	dp->msg_first = sdp->msg_first;
	dp->msg_last = sdp->msg_last;
	dp->msg_cbytes = sdp->msg_cbytes;
	dp->msg_qnum = sdp->msg_qnum;
	dp->msg_qbytes = sdp->msg_qbytes;
	dp->msg_lspid = sdp->msg_lspid;
	dp->msg_lrpid = sdp->msg_lrpid;
	dp->msg_stime = sdp->msg_stime;
	dp->msg_rtime = sdp->msg_rtime;
	dp->msg_ctime = sdp->msg_ctime;
}

void
msqid_ds_out(dp, sdp)
	const struct msqid_ds *dp;
	struct sco_msqid_ds *sdp;
{

	ipc_perm_out(&dp->msg_perm, &sdp->msg_perm);
	sdp->msg_first = dp->msg_first;
	sdp->msg_last = dp->msg_last;
	sdp->msg_cbytes = dp->msg_cbytes;
	sdp->msg_qnum = dp->msg_qnum;
	sdp->msg_qbytes = dp->msg_qbytes;
	sdp->msg_lspid = dp->msg_lspid;
	sdp->msg_lrpid = dp->msg_lrpid;
	sdp->msg_stime = dp->msg_stime;
	sdp->msg_rtime = dp->msg_rtime;
	sdp->msg_ctime = dp->msg_ctime;
}

int
sco_msgctl(msqid, cookie, sdp)
	int msqid;
	int cookie;
	struct sco_msqid_ds *sdp;
{
	struct msqid_ds d;
	int r;

	if (cookie == IPC_SET)
		msqid_ds_in(sdp, &d);
	r = msgctl(msqid, cookie, &d);
	if (r != -1 && cookie == IPC_STAT)
		msqid_ds_out(&d, sdp);
	return (r);
}

int
sco_msg(cookie, a, b, c, d, e)
	int cookie, a, b, c, d, e;
{

	switch (cookie) {
	case SCO_MSGGET:
		return (msgget((long)a, b));
	case SCO_MSGSND:
		return (commit_msgsnd(a, (const void *)b, (size_t)c, d));
	case SCO_MSGRCV:
		return (commit_msgrcv(a, (void *)b, (size_t)c, (long)d, e));
	case SCO_MSGCTL:
		return (sco_msgctl(a, b, (struct sco_msqid_ds *)c));
	}

	errno = EINVAL;
	return (-1);
}
