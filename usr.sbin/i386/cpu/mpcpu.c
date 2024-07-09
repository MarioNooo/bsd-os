/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	mpcpu.c,v 1.17 2002/11/12 15:31:51 dab Exp
 */
#include "cpu.h"

int valid_cpus;
int cpu_min_family;

static void disp_icnts __P((struct ithdinfo *));

/*
 * One-size-fits-all attempt to get all CPU's up and running
 */
int
go_mp(int ac, char **av)
{
	int i;
	struct mce_proc *pp;
	int id;
	int mode;
	int n[3];

	/* Fire up symmetric I/O mode */
	find_valid_cpus();
	if (miscinfo.siomode == 0)
		do_siomode();

	if (vflag)
		printf("\nStarting CPUs:\n");

	/* Start all CPU's the BIOS tells us about */
	for (i = 0; ; i++) {
		if ((pp = get_cpu(i)) == NULL)
			break;
		id = pp->mce_proc_local_apic_id;
		if (!pp->mce_proc_cpu_flags & MCE_PROC_FL_EN) {
			if (vflag)
				printf("BIOS reports cpu %d is disabled\n", id);
			continue;
		}
		if (id < 0 || id >= MAXCPUS) {
			warnx("Can't start cpu %x: out of range", id);
			continue;
		}
		if ((valid_cpus & (1 << id)) == 0)
			continue;
		if (vflag > 1)
			printf("\tCPU %u\n", id);
		if (debug)
			continue;
		n[0] = CTL_MACHDEP;
		n[1] = CPU_CPU;
		n[2] = id;
		mode = CPU_UP;
		if (sysctl(n, 3, NULL, NULL, &mode, sizeof(mode)) < 0)
			warn("cpu-%d: CPU_CPU -> CPU_UP", id);
	}
	return (0);
}

/*
 * Start one CPU
 */
int
cpu_start(int ac, char **av)
{
	int id;
	int n[3];
	int mode;

	if (ac != 2)
		errx(1, "Usage: cpu start cpu_id");
	if (!mp_capable())
		errx(1, "BIOS reports machine is not MP capable");
	id = strtoul(av[1], NULL, 0);
	if (id >= MAXCPUS)
		errx(1, "Cpu number out of range (0-%d)", MAXCPUS - 1);
	n[0] = CTL_MACHDEP;
	n[1] = CPU_CPU;
	n[2] = id;
	mode = CPU_UP;
	if (sysctl(n, 3, NULL, NULL, &mode, sizeof(mode)) < 0) {
		if (errno == ENOENT)
			errx(1, "No such CPU numbered %x", id);
		else
			err(1, "cpu-%x: CPU_CPU -> CPU_UP", id);
	}
	return (0);
}

/*
 * Stop one CPU
 */
int
cpu_stop(int ac, char **av)
{
	int id;
	int n[3];
	int mode;

	if (ac != 2)
		errx(1, "Usage: cpu stop cpu_id");
	if (!mp_capable())
		errx(1, "BIOS reports machine is not MP capable");
	id = strtoul(av[1], NULL, 0);
	if (id >= MAXCPUS)
		errx(1, "Cpu number out of range (0-%d)", MAXCPUS - 1);
	if (id == miscinfo.bootcpu)
		warnx("WARNING: It may not be possible to restart "
		    "the boot cpu");
	n[0] = CTL_MACHDEP;
	n[1] = CPU_CPU;
	n[2] = id;
	mode = CPU_STOP;
	if (sysctl(n, 3, NULL, NULL, &mode, sizeof(mode)) < 0) {
		if (errno == ENOENT)
			errx(1, "No such CPU numbered %x", id);
		else
			err(1, "cpu-%x: CPU_CPU -> CPU_STOP", id);
	}
	return (0);
}

struct ti {
	int mask;
	char *name;
} tit[] = {
	{ KTR_GEN,	"GEN" },
	{ KTR_NET,	"NET" },
	{ KTR_DEV,	"DEV" },
	{ KTR_LOCK,	"LOCK" },
	{ KTR_SMP,	"SMP" },
	{ KTR_FS,	"FS" },
	{ KTR_PMAP,	"PMAP" },
	{ KTR_MALLOC,	"MALLOC" },
	{ KTR_TRAP,	"TRAP" },
	{ KTR_INTR,	"INTR" },
	{ KTR_SIG,	"SIG" },
	{ KTR_CLK,	"CLK" },
	{ KTR_PROC,	"PROC" },
	{ KTR_SYSC,	"SYSC" },
	{ KTR_INIT,	"INIT" },
	{ KTR_KGDB,	"KGDB" },
	{ KTR_IO,	"IO" },
	{ KTR_LOCKMGR,	"LOCKMGR" },
	{ KTR_NFS,	"NFS" },
	{ KTR_VOP,	"VOP" },
	{ KTR_VM,	"VM" },
};

/*
 * Display CPU and interrupt status
 */
int
cpu_stat(int ac, char **av)
{
	int i;
	int t;
	char *m;
	struct ti *ti;

	if (miscinfo.ncpu == 1)
		printf("1 cpu running:\n");
	else
		printf("%d cpus scheduling:\n", miscinfo.ncpu);
	for (i = 0;  i < MAXCPUS; i++) {
		t = miscinfo.cpustate[i];
		if (t == CPU_UNKNOWN)
			continue;
		printf("    Cpu-%x: ", i);
		switch (t) {
		case CPU_UP:
			m = "Running";
			break;
		case CPU_INITAP:
			m = "Initializing";
			break;
		case CPU_STOP:
			m = "Stopped";
			break;
		case CPU_STOPPING:
			m = "Stopping";
			break;
		case CPU_REJECT:
			m = "Failed unique ID test";
			break;
		case CPU_INITERR:
			m = "Failed initialization";
			break;
		case CPU_PANIC_HALT:
			m = "Halted (PANIC)";
			break;
		default:
			printf("(%x) ", t);
			m = "Unknown";
			break;
		}
		if (i == miscinfo.bootcpu)
			printf("%s [boot cpu]\n", m);
		else
			printf("%s\n", m);
	}

	if (miscinfo.siomode)
		printf("\nI/O: APIC mode (SMP compatible)\n");
	else
		printf("\nI/O: 8259 PIC mode (AT compatible)\n");

	printf("\nInterrupt counts:\n");
	disp_icnts(ihead);

	if (vflag) {
		printf("\nTrace mask: 0x%8.8x\n", miscinfo.ktrmask);
		for (ti = tit; ti != &tit[sizeof(tit)/sizeof(struct ti)]; ti++)
			if (ti->mask & miscinfo.ktrmask)
				printf(" %s", ti->name);
		printf("\n\n\n");
		piroute(ihead);
	}
	return (0);
}

static void
disp_icnts(struct ithdinfo *head)
{
	struct ithdinfo *itp;
	ihand_t *ihp;
	size_t buf_len;
	size_t thread_len;
	size_t handler_len;
	int i;
	char buf[60];
	char buf2[60];
	char *thread = "Thread";
	char *handler = "Handler(s)";

	thread_len = strlen(thread);
	handler_len = strlen(handler);
	for (itp = head; itp != NULL; itp = itp->it_next) {
		size_t hl;

		if (strlen(itp->it_name) > thread_len)
			thread_len = strlen(itp->it_name);
		if (itp->it_flag & P_SITHD)
			continue;
		hl = 0;
		LIST_FOREACH(ihp, &itp->it_ihhead, ih_list) {
			if (hl > 0)
				hl++;
			hl += strlen(ihp->ih_xname);
		}
		if (hl > handler_len)
			handler_len = hl;
			
	}

	thread_len += 2;
	handler_len +=2;

	printf("%10s %3s %-*s %-*s %s\n",
	    "Count", "PID", 
	    thread_len, thread, 
	    handler_len, handler, "Source");

	buf_len = thread_len + handler_len + 5;

	for (itp = head; itp != NULL; itp = itp->it_next) {
		isrc_t *isp;

		/* PID and Thread name */
		i = snprintf(buf, sizeof(buf), "%3d %-*s",
		    itp->it_id, thread_len, itp->it_name);

		/* Soft interrupt threads are easy */
		if (itp->it_flag & P_SITHD) {
			printf("%10d %-*s\n", itp->it_cnt, 
			    buf_len, buf);
			continue;
		}

		/* Accumulate list of handlers */
		LIST_FOREACH(ihp, &itp->it_ihhead, ih_list)
			i += snprintf(&buf[i], sizeof(buf) - i, " %s", ihp->ih_xname);

		/* Compute the source and print a line for each one */
		LIST_FOREACH(isp, &itp->it_isrchead, is_tlist) {
			isrc_apic_t *ap;
			isrc_pic_t *pp;

			ap = (isrc_apic_t *)isp;
			switch (isp->is_type) {
			case PicNormal:
				pp = (isrc_pic_t *)isp;
				if (pp->is_irq < 8)
					snprintf(buf2, sizeof(buf2), 
					    "Pic Master input %u",
					    pp->is_irq);
				else
					snprintf(buf2, sizeof(buf2),
					    "Pic Slave input %u",
					    pp->is_irq - 8);
				break;
			case ApicNormal:
				snprintf(buf2, sizeof(buf2), "APIC %u.%u",
				    ap->is_apic, ap->is_pin);
				break;
			case ApicClk:
				snprintf(buf2, sizeof(buf2), "APIC Clock %u.%u",
				    ap->is_apic, ap->is_pin);
				break;
			case ApicClkHack:
				snprintf(buf2, sizeof(buf2),
				    "Apic Clock [PIIX hack] %u.%u",
				    ap->is_apic, ap->is_pin);
				break;
			case ApicUnused:
				snprintf(buf2, sizeof(buf2),
				    "Apic Unused %u.%u",
				    ap->is_apic, ap->is_pin);
				break;
			default:
				snprintf(buf2, sizeof(buf2),
				    "Unknown type (0x%x)", isp->is_type);
				break;
			}
			printf("%10d %-*s %s\n", isp->is_cnt,
			    buf_len, buf, buf2);
			buf[0] = '\0';
		}
	}
}
char ovr_cpu[] = "^cpu" RNUM "=(up|down)$";

/* 
 * Build a bit vector of valid CPUs, start with BIOS info and apply any
 * cpuN=valid/down overrides.
 */
void
find_valid_cpus(void)
{
	int i;
	int num[3];
	char *p;
	struct mce_proc *pp;
	int id;
	int family;

	family = valid_cpus = 0;

	/* Find all BIOS has to tell us about */
	for (i = 0; ; i++) {
		if ((pp = get_cpu(i)) == NULL)
			break;
		id = pp->mce_proc_local_apic_id;
		if (!pp->mce_proc_cpu_flags & MCE_PROC_FL_EN) {
			if (vflag)
				printf("BIOS reports cpu %d is disabled\n", id);
			continue;
		}
		if (id < 0 || id >= MAXCPUS) {
			warnx("BIOS reported Cpu-%x: out of range", id);
			continue;
		}

		/*
		 * Keep track of minimum family, issue a warning if we
		 * detect more than one family.
		 */
		family = CPUID_FAMILY(pp->mce_proc_cpu_signature);
		if (cpu_min_family == 0)
			cpu_min_family = family;
		else if (family < cpu_min_family) {
			warnx("multiple CPU families reported (%x and %x)",
			    cpu_min_family, family);
			cpu_min_family = family;
		}

		valid_cpus |= 1 << id;
	}

	/* Apply overrides */
	for (i = 0; ; i++) {
		if ((p = ovrn(ovr_cpu, i, num)) == NULL)
			break;
		if (num[0] < 0 || num[0] >= MAXCPUS)
			errx(1, "Override for CPU-%x: invalid cpu number",
			    num[0]);
		if (ovr_chk("up", p))
			valid_cpus |= 1 << num[0];
		else
			valid_cpus &= ~(1 << num[0]);
	}
}
