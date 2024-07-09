/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	cpucheck.c,v 1.13 2002/12/18 20:44:10 jch Exp
 */
#define	BRAND_TABLE
#include "cpu.h"

static struct famod {
	int fam;
	int mod;
	char *desc;
} const famod[] = {
	{ 4, 0, "486 DX" },
	{ 4, 1, "486 DX" },
	{ 4, 2, "486 SX" },
	{ 4, 3, "486DX2" },
	{ 4, 4, "486SL" },
	{ 4, 5, "486SX2" },
	{ 4, 7, "486DX2 (writeback enhanced)" },
	{ 4, 8, "486DX4" },
	{ 5, 1, "Pentium 60/66" },
	{ 5, 2, "Pentium 75/90/100/120/133/150/166/200" },
	{ 5, 3, "Pentium overdrive for 486 systems" },
	{ 5, 4, "Pentium MXX 166/200" },
	{ 6, 1, "Pentium Pro" },
	{ 6, 2, "Pentium Pro MMX" },
	{ 6, 3, "Pentium II model 3" },
	{ 6, 5, "Pentium II model 5" },
	{ 6, 6, "Celeron model 6" },
	{ 6, 7, "Pentium III model 7" },
	{ 6, 8, "Pentium III model 8" }
	/* Brand ID and Brand String are used after this */
};
#define	NFAMOD (sizeof(famod) / sizeof(struct famod))

static const char *fbits[] = {
	"Floating-point unit on-chip",
	"Virtual mode extension",
	"Debugging extension",
	"Page size extension",
	"Time stamp counter",
	"Model specific registers",
	"Physical address extension",
	"Machine check exception",
	"CMPXCHG8 instruction supported",
	"On-chip APIC hardware supported",
	"Reserved10",
	"Fast System Call (SYSENTER/SYSEXIT)",
	"Memory Type Range Registers",
	"Page global enable",
	"Machine check architecture",
	"Conditional move instruction support",
	"Page Attribute Table",
	"36-bit Page Size Extension",
	"Processor serial number present and enabled",
	"CLFLUSH instruction supported",
	"Reserved20",
	"Debug Trace Store",
	"ACPI supported",
	"Intel Architecture MMX technology supported",
	"Fast floating point save and restore",
	"Streaming SIMD Extensions supported",
	"Streaming SIMD Extensions 2",
	"Self-Snoop",
	"Hyper-Threading Technology supported",
	"Thermal Monitor supported",
	"Reserved30",
	"Signal Break on FERR",
};

static const char *fbits2[] = {
	"Reserved0",
	"Reserved1",
	"Reserved2",
	"Reserved3",
	"Reserved4",
	"Reserved5",
	"Reserved6",
	"Reserved7",
	"Reserved8",
	"Reserved9",
	"Context ID",
	"Reserved11",
	"Reserved12",
	"Reserved13",
	"Reserved14",
	"Reserved15",
	"Reserved16",
	"Reserved17",
	"Reserved18",
	"Reserved19",
	"Reserved20",
	"Reserved21",
	"Reserved22",
	"Reserved23",
	"Reserved24",
	"Reserved25",
	"Reserved26",
	"Reserved27",
	"Reserved28",
	"Reserved29",
	"Reserved30",
	"Reserved31",
	"Reserved32"
};

static struct cdesc {
	int val;
	char *desc;
} const cdesc[] = {
	{ 0x01, "Instruction TLB: 4K pages, 4-way set associative, 32 entries"},
	{ 0x02, "Instruction TLB: 4M pages, 4-way set associative, 4 entries"},
	{ 0x03, "Data TLB: 4K pages, 4-way set associative, 64 entries"},
	{ 0x04, "Data TLB: 4M pages, 4-way set associative, 8 entries"},
	{ 0x06, "Instruction cache, 32 byte line size, "
	    "4-way set associative, 8K"},
	{ 0x08, "Instruction cache, 32 byte line size, "
	    "4-way set associative, 16K"},
	{ 0x0a, "Data cache, 32 byte line size, "
	    "2-way set associative, 8K"},
	{ 0x0c, "Data cache, 32 byte line size, "
	    "2-way set associative, 16K"},
	{ 0x40, "No L2 cache" },
	{ 0x41, "Unified cache, 32 byte line size, "
	    "4-way set associative, 128K"},
	{ 0x42, "Unified cache, 32 byte line size, "
	    "4-way set associative, 256K"},
	{ 0x43, "Unified cache, 32 byte line size, "
	    "4-way set associative, 512K"},
	{ 0x44, "Unified cache, 32 byte line size, "
	    "4-way set associative, 1M"},
	{ 0x45, "Unified cache, 32 byte line size, "
	    "4-way set associative, 2M"},
	{ 0x50, "Instruction TLB, 4K, 2M or 4M pages, "
	    "fully associative, 64 entries"},
	{ 0x51, "Instruction TLB, 4K, 2M or 4M pages, "
	    "fully associative, 128 entries"},
	{ 0x52, "Instruction TLB, 4K, 2M or 4M pages, "
	    "fully associative, 256 entries"},
	{ 0x5b, "Data TLB, 4K or 4M pages, "
	    "fully associative, 64 entries"},
	{ 0x5c, "Data TLB, 4K or 4M pages, "
	    "fully associative, 128 entries"},
	{ 0x5d, "Data TLB, 4K or 4M pages, "
	    "fully associative, 256 entries"},
	{ 0x66, "Data cache, sectored, 64 byte cache line, "
	    "4 way set associative, 8K"},
	{ 0x67, "Data cache, sectored, 64 byte cache line, "
	    "4 way set associative, 16K"},
	{ 0x68, "Data cache, sectored, 64 byte cache line, "
	    "4 way set associative, 32K"},
	{ 0x70, "Instruction Trace cache, "
	    "4 way set associative, 12K uOps"},
	{ 0x71, "Instruction Trace cache, "
	    "4 way set associative, 16K uOps"},
	{ 0x72, "Instruction Trace cache, "
	    "4 way set associative, 32K uOps"},
	{ 0x79, "Unified cache, sectored, 64 byte cache line, "
	    "8 way set associative, 128K"},
	{ 0x7a, "Unified cache, sectored, 64 byte cache line, "
	    "8 way set associative, 256K"},
	{ 0x7b, "Unified cache, sectored, 64 byte cache line, "
	    "8 way set associative, 512K"},
	{ 0x7c, "Unified cache, sectored, 64 byte cache line, "
	    "8 way set associative, 5121M"},
	{ 0x81, "Unified cache, 32 byte line size, "	/* Guess */
	    "8-way set associative, 128K"},
	{ 0x82, "Unified cache, 32 byte line size, "
	    "8-way set associative, 256K"},
	{ 0x83, "Unified cache, 32 byte line size, "
	    "8-way set associative, 512K"},
	{ 0x84, "Unified cache, 32 byte line size, "
	    "8-way set associative, 1M"},
	{ 0x85, "Unified cache, 32 byte line size, "
	    "8-way set associative, 2M"},
};
#define NCDESC (sizeof(cdesc) / sizeof(struct cdesc))

static int
print_cpu(char *cpu, 
    u_int apicvers, 
    u_int flags, 
    u_int32_t signature,
    u_int32_t feature,
    u_int32_t feature2,
    u_int32_t brandid,
    char *brand,
    u_int32_t apicid,
    u_int32_t nlogproc)
{
	const struct famod *fp;
	int i;
	int type;
	int family;
	int model;
	int stepping;
	char *p;

	type = CPUID_TYPE(signature);
	family = CPUID_FAMILY(signature);
	model = CPUID_MODEL(signature);
	stepping = CPUID_STEPPING(signature);

	if (vflag == 0) {
		if (feature & CPUID_FEAT_APIC) {
			printf("%s WILL support SMP\n", cpu);
			printf("%s stepping %d\n", cpu, stepping);
			return (0);
		} else {
			warnx("%s WILL NOT support SMP (no local APIC)",
			    cpu);
			return (1);
		}
	}

	if (flags != 0 || apicvers != 0) {
		printf("%-16s :", cpu);
		if (flags & ~(MCE_PROC_FL_EN|MCE_PROC_FL_BP))
			printf(" %02x", flags);
		if (flags & MCE_PROC_FL_EN)
			printf(" Enabled");
		if (flags & MCE_PROC_FL_BP)
			printf(" Bootprocessor");
		printf("\n");
		printf("%-16s : %u\n", "APIC version", apicvers);
	}
	printf("%-16s : %8.8x\n", "Signature", signature);
	printf("%-16s : ", "  Type");
	switch (type) {
	case 0:
		p = "Original OEM processor";
		break;
	case 1:
		p = "OverDrive Processor";
		break;
	case 2:
		p = "Dual Processor";
		break;
	default:
		p = "Reserved";
		break;
	}
	printf("%s (%x)\n", p, type);
	printf("%-16s : ", "  Family/Model");
	for (fp = famod; fp != &famod[NFAMOD]; fp++) {
		if (fp->fam == family && fp->mod == model)
			break;
	}
	if (fp == &famod[NFAMOD])
		printf("%x / %x\n", family, model);
	else
		printf("%s (%x / %x)\n", fp->desc, family, model);
	printf("%-16s : %d\n", "  Stepping", stepping);

	if (brand != NULL)
		printf("%-16s : %-.36s\n", "Brand", brand);

	if (brandid > 0 &&
	    (family > CPU_686 ||
		(family == CPU_686 && model >= 8))) {
		struct brand_table *bp;

		printf("%-16s : ", "Brand ID");

		/* Use Intel defined CPU brand string if we recognize it */
		for (bp = intel_brands; bp->br_name != NULL; bp++)
			if (bp->br_id == brandid)
				break;
			
		if (bp->br_name != NULL)
			printf("%s (%u)\n", bp->br_name, brandid);
		else
			printf("%u\n", brandid);
	}

	/* Print APIC ID on P4 processors */
	if (family >= CPU_P4 && apicid != -1)
		printf("%-16s : %d\n", "APIC ID", apicid);

	/* Print Number of logical processors if Hyper-Threading Technology enabled */
	if (feature & CPUID_FEAT_HTT && nlogproc != -1)
		printf("%-16s : %d\n", "Logical Procs", nlogproc);

	printf("%-16s : %8.8x\n", "Features", feature);
	for (i = 0; i < 32; i++)
		if (1 << i & feature)
	printf("%-16s : %8.8x %2d %s\n", "", 1 << i, i, fbits[i]);

	if (feature2) {
		printf("%-16s : %8.8x\n", "Features2", feature2);
		for (i = 0; i < 32; i++)
			if (1 << i & feature2)
				printf("%-16s : %8.8x %2d %s\n", "", 1 << i, i,
				    fbits2[i]);
	}

	return (0);
}

/*
 * Display CPU information from bootparams
 */
int
disp_cpu(int ac, char **av)
{
	struct mce_proc *pp;
	int i;
	int rc;
	char cpuno[32];

	rc = 1;

	/* Display MP data from bootparam to display info */
	for (i = 0; (pp = get_ent_bytype(MCE_PROC, i)) != NULL; i++) {
		if (i != 0)
			printf("\n");
		(void)snprintf(cpuno, sizeof(cpuno), "Cpu-%u",
		    pp->mce_proc_local_apic_id);
		rc |= print_cpu(cpuno,
		    pp->mce_proc_local_apic_vers,
		    pp->mce_proc_cpu_flags,
		    pp->mce_proc_cpu_signature,
		    pp->mce_proc_cpu_feature,
		    0,
		    0,
		    NULL,
		    -1, 
		    -1);
	}

	if (i == 0)
		printf("No MP information in BIOS.\n");

	return (rc);
}

/*
 * Display CPU information
 * 
 * Bit positions are from Intel AP note AP-485 (Intel Processor Identification
 * and the CPUID Instruction).
 */
int
cpu_check(int ac, char **av)
{
	struct cdesc const *cp;
	int apicid;
	int brandid;
	int cpuid;
	int feature;
	int feature2;
	int has_cpuid;
	int hival;
	int i;
	int needed;
	int nlogproc;
	int sig[3];
	char *brand, brandstr[48];

	/* See if we have the CPU ID instruction */
	asm volatile ("pushfl;"
		      "pop %%eax;"
		      "movl %%eax,%%edx;"
		      "xorl $1<<21,%%eax;"	/* Try to flip ID bit */
		      "pushl %%eax;"
		      "popfl;"
		      "pushfl;"
		      "pop %%eax;"
		      "xorl %%edx,%%eax;"
		      "andl $1<<21,%%eax": "=a" (has_cpuid) : : "dx");
	if (!has_cpuid)
		errx(1, "Cpu WILL NOT support SMP (i386)");

	asm volatile ("movl $1,%%eax; cpuid;" : "=a" (cpuid),
	    "=b" (i), "=c" (feature2), "=d" (feature));
	brandid = i & CPUID_MASK_BRAND;
	nlogproc = (i & CPUID_MASK_NLOGPROC) >> CPUID_SHIFT_NLOGPROC;
	apicid = (i & CPUID_MASK_APICID) >> CPUID_SHIFT_APICID;

	/* Get CPU signature */
	asm volatile ("movl $0,%%eax;"
	    "cpuid;"
	    "movl %%ebx,%1;"
	    "movl %%edx,4+%1;"
	    "movl %%ecx,8+%1;" : "=a" (hival),
	    "=m" (sig) : : "bx", "cx", "dx");

	/* First see if we support a Processor Brand String */
	brand = NULL;
	if ((cpuid & CPUID_MASK_MODEL) != 0) {
		u_int max_xtnd = 0;

		asm volatile ("movl %1,%%eax;"
		    "cpuid;"
		    "movl %%eax,%0;" : "=m" (max_xtnd) : "i" (CPUID_BRAND_MAX) : "ax", "bx", "cx", "dx");
		if (max_xtnd >= CPUID_BRAND_STR3) {
			asm volatile ("movl %1,%%eax;"
			    "cpuid;"
			    "movl %%eax,%0;"
			    "movl %%ebx,4+%0;"
			    "movl %%ecx,8+%0;"
			    "movl %%edx,12+%0;" : 
			    	"=m" (brandstr) :
			    	"i" (CPUID_BRAND_STR1) :
				"ax", "bx", "cx", "dx");
			asm volatile ("movl %1,%%eax;"
			    "cpuid;"
			    "movl %%eax,%0;"
			    "movl %%ebx,4+%0;"
			    "movl %%ecx,8+%0;"
			    "movl %%edx,12+%0;" :
			    	"=m" (brandstr[16]) :
				"i" (CPUID_BRAND_STR2) :
			    	"ax", "bx", "cx", "dx");
			asm volatile ("movl %1,%%eax;"
			    "cpuid;"
			    "movl %%eax,%0;"
			    "movl %%ebx,4+%0;"
			    "movl %%ecx,8+%0;"
			    "movl %%edx,12+%0;" :
				"=m" (brandstr[32]) :
			    	"i" (CPUID_BRAND_STR3) :
				"ax", "bx", "cx", "dx");
			brand = brandstr;
		}
	}


	if (vflag)
		printf("%-16s : %12.12s\n", "Vendor ID", (char *)sig);

	i = print_cpu("Cpu", 0, 0, cpuid, feature, feature2, brandid, brand,
	    apicid, nlogproc);

	if (!vflag)
		return (i);

	if (hival >= 3 && feature & CPUID_FEAT_PSN) {
		u_int16_t serial[4];

		/* Get CPU serial number */
		asm volatile ("movl $3,%%eax;"
		    "cpuid;"
		    "movl %%edx,%0;"
		    "movl %%ecx,4+%0;" : "=m" (serial) : :
		    "cx", "dx");
		printf("%-16s : %04X-%04X-%04X-%04X-%04X-%04X\n",
		    "Serial number",
		    cpuid >> 16 & 0xffff, cpuid & 0xffff,
		    serial[0], serial[1], serial[2], serial[3]);
	}

	if (hival < 2)
		return (0);

	printf("Cache information\n");
	needed = 1;
	for (i = 0; i < needed; i++) {
		u_int32_t cinfo[4];
		u_int32_t r;
		u_int j;

		asm volatile ("movl $2,%%eax;"
			      "cpuid;"
			      "movl %%eax,%0;"
			      "movl %%ebx,4+%0;"
			      "movl %%ecx,8+%0;"
			      "movl %%edx,0xc+%0;" : "=m" (cinfo) : :
			      "ax", "bx", "cx", "dx");
		needed = cinfo[0] & 0xff;

		/* Look at each of the registers */
		for (i = 0; i < 4; i++) {
			r = cinfo[i];

			/* If high bit is set we don't understand the format */
			if (r & 0x80000000)
				continue;

			/* 
			 * Look at each of the bytes (except the low
			 * order byte of eax).
			 */
			if (i == 0) {
				j = 1;
				r >>= NBBY;
			} else
				j = 0;
			for (; j < sizeof(r); j++, r >>= NBBY) {
				int cd;

				cd = r & 0xff;
				if (cd == 0)
					continue;
				for (cp = cdesc; cp != &cdesc[NCDESC]; cp++) {
					if (cp->val == cd)
						break;
				}
				if (cp != &cdesc[NCDESC])
					printf("  %s\n", cp->desc);
				else
					printf("  Unknown descriptor: 0x%x\n",
					    cd);
			}
		}

	}

	return (0);
}
