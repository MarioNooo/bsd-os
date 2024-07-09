/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	cpu.h,v 1.13 2003/09/16 01:40:47 jch Exp
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ktr.h>
#include <sys/reboot.h>

#include <i386/machine/apic.h>
#include <i386/machine/bootparam.h>
#include <i386/machine/inline.h>
#include <i386/machine/intel_mps.h>
#include <i386/machine/pmparam.h>
#define KERNEL
#define	_SYS_KTR_H_
#include <i386/machine/pcpu.h>
#undef KERNEL
/*
 * This sooo ungly
 * These are defined in systm.h which won't go with stdio.h because
 * of diffs in printf.
 */
typedef void (*sifunc_t)(void);
typedef struct ithd ithd_t;
#include <sys/sysctl.h>
#include <i386/machine/cpu.h>
#include <i386/isa/isavar.h>
#include <i386/isa/isa.h>
#include <i386/isa/intr.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef int (*cfunc_t)(int ac, char **av);

#define	OVR_FILE "/etc/mp.config"
#define	MAX_OVR 128
#define	RNUM	"(0x[[:xdigit:]]+|[0-9]+)"
#define	VCPU(n)	(valid_cpus & 1 << (n))

#define	EDGE	0
#define	LEVEL	1
#define	HIGH	1
#define	LOW	0

struct pcidata {
	struct pcidata *next;
	struct cpu_pciinfo pi;
};

struct miscinfo {
	int siomode;
	int bootcpu;
	int ncpu;
	int ktrmask;
	int cpustate[MAXCPUS];
};

enum apic_type { APIC_TYPE_UNUSED = 0,
		 APIC_TYPE_LOCAL,
		 APIC_TYPE_IO };

struct apic_entry {
	enum apic_type apic_type;	/* Type of APIC at this address */
	u_int	apic_id;		/* Current ID of this APIC */
	u_int	apic_origid;		/* Initial ID of this APIC */
	u_int32_t	apic_paddr;	/* Physical address of this APIC */
};

extern struct apic_entry apic_table[MAXCPUS];

extern int debug;
extern struct mce_bus defbus;
extern struct ithdinfo *ihead;
extern struct fps *fpsp;
extern size_t fpsp_len;
extern char *hnames[];
extern struct miscinfo miscinfo;
extern struct ithdinfo *miscitp;
extern struct ithdinfo *nit;
extern char ovr_apic[];
extern char ovr_cpu[];
extern char ovr_cpuboot[];
extern char ovr_extclk[];
extern char ovr_extdest[];
extern char ovr_extint[];
extern char ovr_irq[];
extern char ovr_nmi[];
extern char ovr_nmidest[];
extern char ovr_noext[];
extern char ovr_noextclk[];
extern char ovr_noisa[];
extern char ovr_nonmi[];
extern char ovr_nopci[];
extern char ovr_nosmi[];
extern char ovr_notpr[];
extern char ovr_pccard[];
extern char ovr_smi[];
extern char ovr_smidest[];
extern struct pcidata *phead;
extern struct pcidata *phead;
extern int special_hacks;
extern int valid_cpus;
extern int vflag;
extern int cpu_min_family;

#include "proto.h"
