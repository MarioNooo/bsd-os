/*	BSDI ipc.x,v 1.4 2001/10/16 05:31:33 donn Exp	*/

include "types.xh"
include "ipc.xh"

int semop(int semid, void *array, unsigned nops);
int semget(key_t key, int nsems, int f);
int semctl(int semid, int semnum, IPC_STAT, struct semid_ds *buf)
{
	return (__bsdi_syscall(SYS___semctl, semid, semnum, IPC_STAT, &buf));
}
int semctl(int semid, int semnum, IPC_SET, const struct semid_ds *buf)
{
	return (__bsdi_syscall(SYS___semctl, semid, semnum, IPC_SET, &buf));
}
int semctl(int semid, int semnum, ipc_ctl_t cmd, void *arg)
{
	return (__bsdi_syscall(SYS___semctl, semid, semnum, cmd, &arg));
}

int msgsnd(int msqid, void *msgp, size_t msgsz, int msgflg);
int msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
int msgget(key_t key, int msgflg);
int msgctl(int msqid, IPC_STAT, struct msqid_ds *buf);
int msgctl(int msqid, IPC_SET, const struct msqid_ds *buf);
int msgctl(int msqid, ipc_ctl_t cmd, void *arg);

void *shmat(int shmid, void *addr, int f);
int shmdt(void *addr);
int shmget(key_t key, int size, int f);
int shmctl(int shmid, IPC_STAT, struct shmid_ds *buf);
int shmctl(int shmid, IPC_SET, const struct shmid_ds *buf);
int shmctl(int shmid, ipc_ctl_t cmd, void *arg);
