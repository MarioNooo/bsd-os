/*	BSDI rstat_proc.c,v 2.6 2002/01/04 15:18:59 dab Exp	*/

/* @(#)rstat_proc.c	2.2 88/08/01 4.0 RPCSRC */
#ifndef lint
static  char bsdid[] = "@(#)BSDI rstat_proc.c,v 2.6 2002/01/04 15:18:59 dab Exp";
static  char sccsid[] = "@(#)rpc.rstatd.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * rstat service:  built with rstat.x and derived from rpc.rstatd.c
 */
#include <sys/param.h>
#include <sys/user.h>
#include <sys/cpustats.h>
#include <sys/diskstats.h>
#include <sys/ttystats.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/namei.h>

#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <kvm.h>
#include <kvm_stat.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <nlist.h>

#undef TRUE		/* XXX */
#undef FALSE		/* XXX */
#include <net/if.h>
#include <rpc/rpc.h>

#undef CPUSTATES	/* XXX */
#undef DK_NDRIVE	/* XXX */
#undef if_ipackets
#undef if_ierrors
#undef if_oerrors
#undef if_collisions
#undef if_opackets
#include "rstat.h"

#ifdef DEBUG
pid_t fork(void) { return 0; }	/* prevent RPC code from forking */
#endif

static kvm_t *kd;		/* /dev/kmem info */
static int stathz;		/* rate for cpu, disk stats (Hz) */
static int dk_cnt;		/* number of disk drives */
static struct diskstats *dbuf;	/* space for disk stats */

int stats_service();

int sincelastreq = 0;		/* number of alarms since last request */
#define CLOSEDOWN 20		/* how long to wait before exiting */
int closedown = CLOSEDOWN;

union {
    struct stats s1;
    struct statsswtch s2;
    struct statstime s3;
} stats_all;

void setup();
void updatestat();
static stat_state = 0;
extern int errno;

void
stat_update()
{
    if (stat_state == 0) {
	    stat_state = 1;
	    setup();
	    signal(SIGALRM, updatestat);
    }
    sincelastreq = 0;
    if (stat_state == 1) 
	    updatestat();
}

statstime *
rstatproc_stats_3_svc(void *argp, struct svc_req *clnt)
{
    stat_update();
    return(&stats_all.s3);
}

statsswtch *
rstatproc_stats_2_svc(void *argp, struct svc_req *clnt)
{
    stat_update();
    return(&stats_all.s2);
}

stats *
rstatproc_stats_1_svc(void *argp, struct svc_req *clnt)
{
    stat_update();
    return(&stats_all.s1);
}

/*
 * returns true if found any disks
 */
int havedisk()
{
	return (dk_cnt != 0);
}

u_int *
rstatproc_havedisk_3_svc(void *argp, struct svc_req *clnt)
{
    static u_int have;

    stat_update();
    have = havedisk();
	return(&have);
}

u_int *
rstatproc_havedisk_2_svc(void *argp, struct svc_req *clnt)
{
    return(rstatproc_havedisk_3_svc(argp, clnt));
}

u_int *
rstatproc_havedisk_1_svc(void *argp, struct svc_req *clnt)
{
    return(rstatproc_havedisk_3_svc(argp, clnt));
}

static void
disk_stats(int *dk_xfer)
{
	int i;
	struct diskstats *dk;

	if (kvm_disks(kd, dbuf, dk_cnt) != dk_cnt)
		errx(1, "disk statistics mismatch (restart rstatd?)");
	for (i = 0, dk = dbuf; i < dk_cnt; i++, dk++)
		dk_xfer[i] = dk->dk_xfers;
}

/*
 * read cpu stats, transform to sun rpc format
 */
static void
cpu_stats(int *cp)
{
	struct cpustats cpustats;

	if (kvm_cpustats(kd, &cpustats) < 0)
		errx(1, "kvm_statcpu: %s", kvm_geterr(kd));
	cp[0] = cpustats.cp_time[CP_USER];
	cp[1] = cpustats.cp_time[CP_NICE];
	cp[2] = cpustats.cp_time[CP_SYS];
	cp[3] = cpustats.cp_time[CP_IDLE];

	/* count interrupt time as generic system time */
	cp[2] += cpustats.cp_time[CP_INTR];
}

static void
vm_stats(struct vmmeter *sum)
{

	if (kvm_vmmeter(kd, sum) < 0)
		errx(1, "kvm_vmmeter: %s", kvm_geterr(kd));
}

void
setup()
{
	char **names;
	char buf[_POSIX2_LINE_MAX];

	if ((kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, buf)) == NULL)
		errx(1, "kvm_openfiles: %s", buf);

	/* Just want to count the disks. */
	names = kvm_dknames(kd, &dk_cnt);

	if (dk_cnt > RSTAT_DK_NDRIVE)
		dk_cnt = RSTAT_DK_NDRIVE;
	
	if ((dbuf = malloc((dk_cnt) * sizeof *dbuf)) == NULL)
		err(1, "malloc");

	/* We assume stathz will not change. */
	stathz = kvm_stathz(kd);
}

void
updatestat()
{
	struct vmmeter sum;
	double avrun[3];
	struct timeval tm, btm;
	struct ifaddrs *ifaddrs, *ifap;
	struct if_data *ifdp;

#ifdef DEBUG
	fprintf(stderr, "entering updatestat\n");
#endif
	sincelastreq++;
	if (sincelastreq > closedown) {
		stat_state = 1;
		return;
	}
	stat_state = 2;
	kvm_boottime(kd, &btm);
	stats_all.s2.boottime.tv_sec = btm.tv_sec;
	stats_all.s2.boottime.tv_usec = btm.tv_usec;

	cpu_stats(stats_all.s1.cp_time);

	(void)getloadavg(avrun, sizeof(avrun) / sizeof(avrun[0]));
	stats_all.s2.avenrun[0] = avrun[0] * RSTAT_FSCALE;
	stats_all.s2.avenrun[1] = avrun[1] * RSTAT_FSCALE;
	stats_all.s2.avenrun[2] = avrun[2] * RSTAT_FSCALE;

	vm_stats(&sum);
	stats_all.s1.v_pgpgin = sum.v_pgpgin;
	stats_all.s1.v_pgpgout = sum.v_pgpgout;
	stats_all.s1.v_pswpin = sum.v_pswpin;
	stats_all.s1.v_pswpout = sum.v_pswpout;
	stats_all.s1.v_intr = sum.v_intr;
	/* XXX the following attempts to subtract away the clock itself,
	   but this is not 100% correct and could even be negative */
	gettimeofday(&tm, NULL);
	stats_all.s3.curtime.tv_sec = tm.tv_sec;
	stats_all.s3.curtime.tv_usec = tm.tv_usec;
	stats_all.s1.v_intr -= stathz*(tm.tv_sec - btm.tv_sec) +
	    stathz*(tm.tv_usec - btm.tv_usec)/1000000;
	stats_all.s2.v_swtch = sum.v_swtch;

	disk_stats(stats_all.s1.dk_xfer);

	stats_all.s1.if_ipackets = 0;
	stats_all.s1.if_opackets = 0;
	stats_all.s1.if_ierrors = 0;
	stats_all.s1.if_oerrors = 0;
	stats_all.s1.if_collisions = 0;
	if (getifaddrs(&ifaddrs) < 0)
		err(1, "can't read interface list");
	for (ifap = ifaddrs; ifap != NULL; ifap = ifap->ifa_next) {
		if (ifap->ifa_addr->sa_family != AF_LINK)
			continue;
		ifdp = (struct if_data *)ifap->ifa_data;
		stats_all.s1.if_ipackets += ifdp->ifi_ipackets;
		stats_all.s1.if_opackets += ifdp->ifi_opackets;
		stats_all.s1.if_ierrors += ifdp->ifi_ierrors;
		stats_all.s1.if_oerrors += ifdp->ifi_oerrors;
		stats_all.s1.if_collisions += ifdp->ifi_collisions;
	}
	freeifaddrs(ifaddrs);

	alarm(1);
}
