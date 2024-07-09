/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI ipcs.c,v 1.2 1996/07/22 17:56:19 mdickson Exp 
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#define KERNEL
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#undef KERNEL

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

struct semid_ds *sema;
struct msqid_ds *msqids;
struct shmid_ds *shmsegs;
struct seminfo seminfo;
struct msginfo msginfo;
struct shminfo shminfo;

void usage __P((void));
void do_sysvshm __P((int, int));
void do_sysvsem __P((int, int));
void do_sysvmsg __P((int, int));

/*
 * Fetch sysv ipc info using the sysctl interface
 */
int
get_ipcinfo(which, where, len)
	int which;
	void *where;
	size_t len;
{
	int mib[3];

	mib[0] = CTL_KERN;
	mib[1] = KERN_SYSVIPC;
	mib[2] = which;

	return (sysctl(mib, 3, where, &len, NULL, 0));
}

char *
fmt_perm(mode)
	u_short mode;
{
	static char buffer[100];

	buffer[0] = '-';
	buffer[1] = '-';
	buffer[2] = ((mode & 0400) ? 'r' : '-');
	buffer[3] = ((mode & 0200) ? 'w' : '-');
	buffer[4] = ((mode & 0100) ? 'a' : '-');
	buffer[5] = ((mode & 0040) ? 'r' : '-');
	buffer[6] = ((mode & 0020) ? 'w' : '-');
	buffer[7] = ((mode & 0010) ? 'a' : '-');
	buffer[8] = ((mode & 0004) ? 'r' : '-');
	buffer[9] = ((mode & 0002) ? 'w' : '-');
	buffer[10] = ((mode & 0001) ? 'a' : '-');
	buffer[11] = '\0';
	return (&buffer[0]);
}

void
cvt_time(t, buf)
	time_t  t;
	char   *buf;
{
	struct tm *tm;

	if (t == 0) 
		strcpy(buf, "no-entry");
	else {
		tm = localtime(&t);
		sprintf(buf, "%2d:%02d:%02d",
		    tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
}

#define	SHMINFO		1
#define	SHMTOTAL	2
#define	MSGINFO		4
#define	MSGTOTAL	8
#define	SEMINFO		16
#define	SEMTOTAL	32

#define BIGGEST		1
#define CREATOR		2
#define OUTSTANDING	4
#define PID		8
#define TIME		16

void
do_sysvmsg(display, option)
	int display;
	int option;
{
	struct msqid_ds *xmsqids;
	char stime_buf[100], rtime_buf[100], ctime_buf[100];
	struct msqid_ds *msqptr;
	int i;

	if (get_ipcinfo(SYSVIPC_MSGINFO, &msginfo, sizeof(msginfo)) != 0) {
		warn("could not retrieve msginfo\n");
		return;
	}

	if (display & MSGTOTAL) {
		printf("msginfo:\n");
		printf("\tmsgmax: %6d\t(max characters in a message)\n",
		    msginfo.msgmax);
		printf("\tmsgmni: %6d\t(# of message queues)\n",
		    msginfo.msgmni);
		printf("\tmsgmnb: %6d\t(max characters in a message queue)\n",
		    msginfo.msgmnb);
		printf("\tmsgtql: %6d\t(max # of messages in system)\n",
		    msginfo.msgtql);
		printf("\tmsgssz: %6d\t(size of a message segment)\n",
		    msginfo.msgssz);
		printf("\tmsgseg: %6d\t(# of message segments in system)\n\n",
		    msginfo.msgseg);
	}

	if ((display & MSGINFO) == 0)
		return;	

	/* Print the details about the message queues */
	printf("Message Queues:\n");
	printf("T           ID          KEY        MODE    OWNER    GROUP");
	if (option & CREATOR)
		printf("  CREATOR   CGROUP");
	if (option & OUTSTANDING)
		printf(" CBYTES  QNUM");
	if (option & BIGGEST)
		printf(" QBYTES");
	if (option & PID)
		printf("   LSPID  LRPID");
	if (option & TIME)
		printf("    STIME    RTIME    CTIME");
	printf("\n");

	xmsqids = calloc(msginfo.msgmni, sizeof(struct msqid_ds));
	if (get_ipcinfo(SYSVIPC_MSQIDS, xmsqids, 
			sizeof(struct msqid_ds) * msginfo.msgmni) != 0) {
		printf("\n");
		return;
	}

	for (i = 0; i < msginfo.msgmni; i += 1) {
		if (xmsqids[i].msg_qbytes == 0) 
			continue;

		msqptr = &xmsqids[i];
		cvt_time(msqptr->msg_stime, stime_buf);
		cvt_time(msqptr->msg_rtime, rtime_buf);
		cvt_time(msqptr->msg_ctime, ctime_buf);

		printf("q %12d %12d %s %8s %8s",
			    IXSEQ_TO_IPCID(i, msqptr->msg_perm),
			    msqptr->msg_perm.key,
			    fmt_perm(msqptr->msg_perm.mode),
			    user_from_uid(msqptr->msg_perm.uid, 0),
			    group_from_gid(msqptr->msg_perm.gid, 0));

		if (option & CREATOR)
			printf(" %8s %8s", 
				user_from_uid(msqptr->msg_perm.cuid, 0),
				group_from_gid(msqptr->msg_perm.cgid, 0));

		if (option & OUTSTANDING)
			printf(" %6d %6d", 
				msqptr->msg_cbytes, msqptr->msg_qnum);

		if (option & BIGGEST)
			printf(" %6d", msqptr->msg_qbytes);

		if (option & PID)
			printf(" %6d %6d", 
				msqptr->msg_lspid, msqptr->msg_lrpid);

		if (option & TIME)
			printf(" %s %s %s", stime_buf, rtime_buf, ctime_buf);

		printf("\n");
	}
	printf("\n");
}

void
do_sysvshm(display, option)
	int display;
	int option;
{
	struct shmid_ds *xshmids;
	char atime_buf[100], dtime_buf[100], ctime_buf[100];
	struct shmid_ds *shmptr;
	int i;

	if (get_ipcinfo(SYSVIPC_SHMINFO, &shminfo, sizeof(shminfo)) != 0) {
		warn("could not retrieve shminfo\n");
		return;
	}

	if (display & SHMTOTAL) {
		printf("shminfo:\n");
		printf("\tshmmax: %7d\t(max shared memory segment size)\n",
		    shminfo.shmmax);
		printf("\tshmmin: %7d\t(min shared memory segment size)\n",
		    shminfo.shmmin);
		printf("\tshmmni: %7d\t(max number of shared memory identifiers)\n",
		    shminfo.shmmni);
		printf("\tshmseg: %7d\t(max shared memory segments per process)\n",
		    shminfo.shmseg);
		printf("\tshmall: %7d\t(max amount of shared memory in pages)\n\n",
		    shminfo.shmall);
	}

	if ((display & SHMINFO) == 0)
		return;

	/* Display the details */
	printf("Shared Memory:\n");
	printf("T           ID          KEY        MODE    OWNER    GROUP");
	if (option & CREATOR)
		printf("  CREATOR   CGROUP");
	if (option & OUTSTANDING)
		printf(" NATTCH");
	if (option & BIGGEST)
		printf("  SEGSZ");
	if (option & PID)
		printf("   CPID   LPID");
	if (option & TIME)
		printf("    ATIME    DTIME    CTIME");
	printf("\n");

	xshmids = calloc(shminfo.shmmni, sizeof(struct shmid_ds));
	if (get_ipcinfo(SYSVIPC_SHMSEGS, xshmids, 
		sizeof(struct shmid_ds) * shminfo.shmmni) != 0) {
		printf("\n");
		return;
	}

	for (i = 0; i < shminfo.shmmni; i += 1) {
		if ((xshmids[i].shm_perm.mode & 0x0800) == 0)
			continue;
		shmptr = &xshmids[i];
		cvt_time(shmptr->shm_atime, atime_buf);
		cvt_time(shmptr->shm_dtime, dtime_buf);
		cvt_time(shmptr->shm_ctime, ctime_buf);

		printf("m %12d %12d %s %8s %8s",
		    IXSEQ_TO_IPCID(i, shmptr->shm_perm),
		    shmptr->shm_perm.key,
		    fmt_perm(shmptr->shm_perm.mode),
		    user_from_uid(shmptr->shm_perm.uid, 0),
		    group_from_gid(shmptr->shm_perm.gid, 0));

		if (option & CREATOR)
			printf(" %8s %8s",
			    user_from_uid(shmptr->shm_perm.cuid, 0),
			    group_from_gid(shmptr->shm_perm.cgid, 0));

		if (option & OUTSTANDING)
			printf(" %6d", shmptr->shm_nattch);

		if (option & BIGGEST)
			printf(" %6d", shmptr->shm_segsz);

		if (option & PID)
			printf(" %6d %6d", shmptr->shm_cpid, shmptr->shm_lpid);

		if (option & TIME)
			printf(" %s %s %s", atime_buf, dtime_buf, ctime_buf);

		printf("\n");
	}
	printf("\n");
}

void
do_sysvsem(display, option)
	int display;
	int option;
{
	struct semid_ds *xsema;
	char ctime_buf[100], otime_buf[100];
	struct semid_ds *semaptr;
	int i, j, value;

	if (get_ipcinfo(SYSVIPC_SEMINFO, &seminfo, sizeof(seminfo)) != 0) {
		warn("could not retrieve seminfo\n");
		return;
	}

	if (display & SEMTOTAL) {
		printf("seminfo:\n");
		printf("\tsemmap: %6d\t(# of entries in semaphore map)\n",
		    seminfo.semmap);
		printf("\tsemmni: %6d\t(# of semaphore identifiers)\n",
		    seminfo.semmni);
		printf("\tsemmns: %6d\t(# of semaphores in system)\n",
		    seminfo.semmns);
		printf("\tsemmnu: %6d\t(# of undo structures in system)\n",
		    seminfo.semmnu);
		printf("\tsemmsl: %6d\t(max # of semaphores per id)\n",
		    seminfo.semmsl);
		printf("\tsemopm: %6d\t(max # of operations per semop call)\n",
		    seminfo.semopm);
		printf("\tsemume: %6d\t(max # of undo entries per process)\n",
		    seminfo.semume);
		printf("\tsemusz: %6d\t(size in bytes of undo structure)\n",
		    seminfo.semusz);
		printf("\tsemvmx: %6d\t(semaphore maximum value)\n",
		    seminfo.semvmx);
		printf("\tsemaem: %6d\t(adjust on exit max value)\n\n",
		    seminfo.semaem);
	}

	if ((display & SEMINFO) == 0)
		return;

	/* Print the details */
	printf("Semaphores:\n");
	printf("T           ID          KEY        MODE    OWNER    GROUP");
	if (option & CREATOR)
		printf("  CREATOR   CGROUP");
	if (option & BIGGEST)
		printf("  NSEMS");
	if (option & TIME)
		printf("    OTIME    CTIME");
	printf("\n");

	xsema = calloc(seminfo.semmni, sizeof(struct semid_ds));
	if (get_ipcinfo(SYSVIPC_SEMIDS, xsema, 
		sizeof(struct semid_ds) * seminfo.semmni) != 0) {
		printf("\n");
		return;
	}

	for (i = 0; i < seminfo.semmni; i += 1) {
		if ((xsema[i].sem_perm.mode & SEM_ALLOC) == 0) 
			continue;

		semaptr = &xsema[i];
		cvt_time(semaptr->sem_otime, otime_buf);
		cvt_time(semaptr->sem_ctime, ctime_buf);

		printf("s %12d %12d %s %8s %8s",
		    IXSEQ_TO_IPCID(i, semaptr->sem_perm),
		    semaptr->sem_perm.key,
		    fmt_perm(semaptr->sem_perm.mode),
		    user_from_uid(semaptr->sem_perm.uid, 0),
		    group_from_gid(semaptr->sem_perm.gid, 0));

		if (option & CREATOR)
			printf(" %8s %8s",
			    user_from_uid(semaptr->sem_perm.cuid, 0),
			    group_from_gid(semaptr->sem_perm.cgid, 0));

		if (option & BIGGEST)
			printf(" %6d", semaptr->sem_nsems);

		if (option & TIME)
			printf(" %s %s", otime_buf, ctime_buf);

		printf("\n");
	}
	printf("\n");
}

int
main(argc, argv)
	int     argc;
	char   *argv[];
{
	int     display = SHMINFO | MSGINFO | SEMINFO;
	int     option = 0;
	int     i;

	while ((i = getopt(argc, argv, "MmQqSsabC:cN:optT")) != EOF) {
		switch (i) {
		case 'M':
			display = SHMTOTAL;
			break;
		case 'm':
			display = SHMINFO;
			break;
		case 'Q':
			display = MSGTOTAL;
			break;
		case 'q':
			display = MSGINFO;
			break;
		case 'S':
			display = SEMTOTAL;
			break;
		case 's':
			display = SEMINFO;
			break;
		case 'T':
			display = SHMTOTAL | MSGTOTAL | SEMTOTAL;
			break;
		case 'a':
			option |= BIGGEST | CREATOR | OUTSTANDING | PID | TIME;
			break;
		case 'b':
			option |= BIGGEST;
			break;
		case 'c':
			option |= CREATOR;
			break;
		case 'o':
			option |= OUTSTANDING;
			break;
		case 'p':
			option |= PID;
			break;
		case 't':
			option |= TIME;
			break;
		default:
			usage();
		}
	}

	do_sysvmsg(display, option);
	do_sysvshm(display, option);
	do_sysvsem(display, option);

	exit(0);
}

void
usage()
{

	fprintf(stderr,
	    "usage: ipcs [-abcmopqst]\n");
	exit(1);
}
