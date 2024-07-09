/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	mpinfo.c,v 1.9 2001/11/05 23:06:42 polk Exp
 */
#include "cpu.h"

struct fps fakefps;
struct fps *fpsp;
size_t fpsp_len;
struct mct *mctp;

void
get_mptable(void)
{

	/* If no bootparams, use a default configuration */
	if (fpsp == NULL) {
		fakefps.fps_cfgtype = 0xff;
		strncpy(fakefps.u.fps_uc_sign, FPS_CHAR_SIGN, sizeof(fakefps.fps_cfgtype));
		fpsp = &fakefps;
		fpsp_len = sizeof(fakefps);
		return;
	}
	if (strncmp(fpsp->fps_c_sign, FPS_CHAR_SIGN, 4))
		errx(1, "Bad FPS signature: %4.4s", fpsp->fps_c_sign);
	switch (fpsp->fps_cfgtype) {
	case 5:
	case 6:
		if (fpsp->fps_cfg == NULL)
			break;
		if (!ovr("ignore_defcfg")) {
			if (vflag)
				warnx("BIOS indicates default MP config type "
				    "but supplies detailed config data\n"
				    "Ignoring detailed BIOS config data (use "
				    "ignore_defcfg to ignore default config)");
			break;
		}
		fpsp->fps_cfgtype = 0;
		/* Fall through if we want to ignore default config types */
	case 0:
		mctp = (struct mct *)(fpsp + 1);
		break;
	default:
		errx(1, "Unsupported FPS configuration: %d");
	}
}

/*
 * Find physical address of local apic on a given CPU
 */
u_long
get_local_apic_addr(int cno)
{
	struct mce_proc *pp = (struct mce_proc *)mctp + 1;
	int i;
	int fam;

	if (!VCPU(cno))
		return (0);
	if (fpsp->fps_cfgtype != 0)
		return (APIC_LOCAL_PA);

	for (i = 0; ; i++) {
		if ((pp = get_ent_bytype(MCE_PROC, i)) == NULL)
			return (APIC_LOCAL_PA);
		if (pp->mce_proc_local_apic_id == cno)
			break;
	}
	fam = pp->mce_proc_cpu_signature >> 8 & 0xf;
	if (fam < 5) {
		warnx("Cpu-%x not a supported processor family (%d)",
		    pp->mce_proc_local_apic_id, fam);
		return (0);
	}
	return ((u_long)mctp->mct_local_apic);
}

struct mce_io_apic defioapic = {
	MCE_IO_APIC,
	2,			/* CPU's are 0 and 1 */
	0x11,			/* version */
	MCE_IO_APIC_FL_EN,	/* enabled */
	(caddr_t) APIC_IO_PA	/* physical address */
};

struct mce_io_apic ovrapic = {
	MCE_IO_APIC,
	2,
	0x11,			/* version */
	MCE_IO_APIC_FL_EN,	/* enabled */
	(caddr_t) APIC_IO_PA	/* physical address */
};

char ovr_apic[] = "^apic" RNUM "=(" RNUM "|down)$";

/*
 * Find an I/O apic by its APIC ID
 */
struct mce_io_apic *
get_io_apic(int id)
{
	struct mce_io_apic *pp;
	int i;
	char *o;
	int n[3];

	for (i = 0; (o = ovrn(ovr_apic, i, n)) != NULL ; i++)
		if (n[0] == id)
			break;
	if (o != NULL) {
		if (ovr_chk("down", o))
			return (NULL);
		if (VCPU(n[0]))
			errx(1, "APIC override invalid, ID %x is a CPU", id);
		if (n[1] & 0xfff)
			errx(1, "APIC physical address (%x) not page aligned",
			    n[1]);
		ovrapic.mce_io_apic_id = id;
		ovrapic.mce_io_apic_addr = (void *)n[1];
		return (&ovrapic);
	}
	if (fpsp->fps_cfgtype != 0) {
		if (id == 2)
			return (&defioapic);
		else
			return (NULL);
	}

	for (i = 0; ; i++) {
		if ((pp = get_ent_bytype(MCE_IO_APIC, i)) == NULL)
			return (NULL);
		if (pp->mce_io_apic_id == id)
			break;
	}
	if (!(pp->mce_io_apic_flags & MCE_IO_APIC_FL_EN)) {
		warnx("IO APIC %d marked disabled by BIOS", id);
		return (NULL);
	}
	return (pp);
}


#define IENT(irq, pin) \
{ MCE_IO_INTR,	MCE_INTR_TYPE_INT, MCE_PO_CONFORM | MCE_FL_CONFORM, \
    0, irq, 2, pin },
struct mce_io_intr defintr[] = {
	IENT(0, 2)
	IENT(1, 1)
	IENT(3, 3)
	IENT(4, 4)
	IENT(5, 5)
	IENT(6, 6)
	IENT(7, 7)
	IENT(8, 8)
	IENT(9, 9)
	IENT(10, 10)
	IENT(11, 11)
	IENT(12, 12)
	IENT(13, 13)
	IENT(14, 14)
	IENT(15, 15)
};

#define	NDEFINTR	(sizeof(defintr) / sizeof(struct mce_io_intr))

/*
 * Get interrupt entry N (counted from first entry, NOT IRQ)
 */
struct mce_io_intr *
get_intr(int n)
{
	if (fpsp->fps_cfgtype != 0) {
		if (n >= (int)NDEFINTR)
			return (NULL);
		return (&defintr[n]);
	}
	return (get_ent_bytype(MCE_IO_INTR, n));
}

/*
 * Get a PCI interrupt entry
 */
struct mce_io_intr *
get_pci_intr(int n, struct mce_bus **buspp)
{
	int i;
	struct mce_io_intr *mip;
	struct mce_bus *busp;

	for (i = 0; (mip = get_intr(i)) != NULL; i++) {
		if (mip->mce_io_intr_type != MCE_INTR_TYPE_INT)
			continue;
		if ((busp = get_bus(mip->mce_io_intr_src_bus_id)) == NULL) {
			warnx("BIOS Interrupt apic=%x pin=%x refers to invalid"
			    " bus id %d (ignored)",
			    mip->mce_io_intr_dst_apic_id,
			    mip->mce_io_intr_dst_apic_int,
			    mip->mce_io_intr_src_bus_id);
			continue;
		}
		if (strncmp(busp->mce_bus_type, "PCI   ", 6) != 0)
			continue;
		if (n-- == 0) {
			*buspp = busp;
			break;
		}
	}
	return (mip);
}

/*
 * Get an ISA/EISA interrupt entry
 */
struct mce_io_intr *
get_isa_intr(int n)
{
	int i;
	struct mce_io_intr *mip;
	struct mce_bus *busp;

	for (i = 0; (mip = get_intr(i)) != NULL; i++) {
		if (mip->mce_io_intr_type != MCE_INTR_TYPE_INT)
			continue;
		if ((busp = get_bus(mip->mce_io_intr_src_bus_id)) == NULL) {
			warnx("BIOS Interrupt apic=%x pin=%x refers to invalid"
			    " bus id %d (ignored)",
			    mip->mce_io_intr_dst_apic_id,
			    mip->mce_io_intr_dst_apic_int,
			    mip->mce_io_intr_src_bus_id);
			continue;
		}
		if (strncmp(busp->mce_bus_type, "ISA   ", 6) != 0 &&
		    strncmp(busp->mce_bus_type, "EISA  ", 6) != 0)
			continue;
		if (n-- == 0)
			break;
	}
	return (mip);
}

struct mce_io_intr defioextint = {
	MCE_IO_INTR,
	MCE_INTR_TYPE_EXTINT,
	MCE_PO_HIGH | MCE_FL_EDGE,
	0,			/* bus */
	0,			/* src irq */
	2,			/* dest IO apic ID */
	0			/* dest pin */
};

/*
 * Get APIC extint entry
 */
struct mce_io_intr *
get_apic_extint(int n)
{
	int i;
	struct mce_io_intr *ip;

	if (fpsp->fps_cfgtype != 0) {
		if (n == 0)
			return (&defioextint);
		else
			return (NULL);
	}
	for (i = 0; (ip = get_ent_bytype(MCE_IO_INTR, i)) != NULL; i++)
		if (ip->mce_io_intr_type == MCE_INTR_TYPE_EXTINT && n-- == 0)
			break;
	return (ip);
}

struct mce_lc_intr deflcintr = {
	MCE_LC_INTR,
	MCE_INTR_TYPE_NMI,
	0,			/* No flags */
	0,			/* bus */
	0,			/* src irq */
	0xff,			/* dest apic ID */
	1			/* target LINTn */
};

/* 
 * Get local interrupt entry N
 */
struct mce_lc_intr *
get_local_intr(int n)
{
	if (fpsp->fps_cfgtype != 0) {
		if (n == 0)
			return (&deflcintr);
		else
			return (NULL);
	}
	return (get_ent_bytype(MCE_LC_INTR, n));
}

struct mce_lc_intr deflcextint = {
	MCE_LC_INTR,
	MCE_INTR_TYPE_EXTINT,
	MCE_PO_HIGH | MCE_FL_EDGE,
	0,			/* bus */
	0,			/* src irq */
	0xff,			/* dest LCL apic ID */
	0			/* dest LINTIN */
};

/*
 * Get local APIC extint entry
 */
struct mce_lc_intr *
get_lcl_extint(int n)
{
	int i;
	struct mce_lc_intr *ip;

	if (fpsp->fps_cfgtype != 0) {
		if (n == 0)
			return (&deflcextint);
		else
			return (NULL);
	}
	for (i = 0; (ip = get_ent_bytype(MCE_LC_INTR, i)) != NULL; i++)
		if (ip->mce_lc_intr_type == MCE_INTR_TYPE_EXTINT && n-- == 0)
			break;
	return (ip);
}

struct mce_bus defbus = {
	MCE_BUS,
	0,			/* bus id */
	"ISA   "		/* bus type */
};

/*
 * Get a bus structure given its ID
 */
struct mce_bus *
get_bus(int n)
{
	int i;
	struct mce_bus *pp;

	if (fpsp->fps_cfgtype != 0) {
		if (n == 0)
			return (&defbus);
		else
			return (NULL);
	}
	for (i = 0; ; i++) {
		if ((pp = get_ent_bytype(MCE_BUS, i)) == NULL)
			return (NULL);
		if (pp->mce_bus_id == n)
			break;
	}
	return (pp);
}

/*
 * Search for an entry of a given type in the resident MPS data
 */
void *
get_ent_bytype(int type, int num)
{
	struct mce_proc *pp = (struct mce_proc *)(mctp + 1);
	int i;
	int len;

	for (i = mctp->mct_entry_cnt; i; i--) {
		if (pp->mce_entry_type == type) {
			if (num == 0)
				return (pp);
			num--;
		}
		switch (pp->mce_entry_type) {
		case MCE_PROC:
			len = MCE_PROC_LEN;
			break;
		case MCE_BUS:
			len = MCE_BUS_LEN;
			break;
		case MCE_IO_APIC:
			len = MCE_IO_APIC_LEN;
			break;
		case MCE_IO_INTR:
			len = MCE_IO_INTR_LEN;
			break;
		case MCE_LC_INTR:
			len = MCE_LC_INTR_LEN;
			break;
		default:
			errx(1, "Invalid BIOS MPS entry type (%d)",
			    pp->mce_entry_type);
		}
		pp = (struct mce_proc *)((char *)pp + len);
	}
	return (NULL);
}

/*
 * Determine if machine is MP capable
 */
int
mp_capable(void)
{
	return (fpsp->fps_cfgtype != 0xff);
}

/* 
 * Determine if IMCR poking must be done
 */
int
check_for_imcr(void)
{
	if (fpsp->fps_feature[0] & FPS_IMCRP)
		return (CPU_IMCR);
	else
		return (0);
}


char ovr_cpuboot[] = "^cpu" RNUM "=boot$";

/*
 * Get boot processor ID
 */
int
get_boot_processor(void)
{
	struct mce_proc *pp;
	int i;
	int n[3];
	char *o;

	if ((o = ovrn(ovr_cpuboot, 0, n)) != NULL) {
		if (ovrn(ovr_cpuboot, 1, n))
			errx(1, "Only one \"cpuN=boot\" allowed");
		if (!VCPU(n[0]))
			errx(1, "%s: Cpu-%x is not valid", o, n[0]);
		return (n[0]);
	}
	if (fpsp->fps_cfgtype != 0) {
		/*
		 * XXX this is kind of bogus, the 1.4 spec doesn't really
		 *     say which ID is the BSP
		 */
		return (0);
	}
	for (i = 0; (pp = get_ent_bytype(MCE_PROC, i)) != NULL; i++) {
		if (pp->mce_proc_cpu_flags & MCE_PROC_FL_BP)
			break;
	}
	if (pp == NULL)
		errx(1, "BIOS did not define a boot processor");
	return (pp->mce_proc_local_apic_id);
}

struct mce_proc def_proc = {
	MCE_PROC,
	0,
	0,
	MCE_PROC_FL_EN,
	0,
	0,
	0,
	0
};

/*
 * Get a CPU record
 */
struct mce_proc *
get_cpu(int n)
{

	if (fpsp->fps_cfgtype == 0xff)
		return (NULL);
	if (fpsp->fps_cfgtype != 0) {
		if (n >= 2)
			return (NULL);
		if (n == 0)
			def_proc.mce_proc_cpu_flags = 
			    MCE_PROC_FL_EN | MCE_PROC_FL_BP;
		else
			def_proc.mce_proc_cpu_flags = MCE_PROC_FL_EN;
		def_proc.mce_proc_local_apic_id = n;
		return (&def_proc);
	}
	return (get_ent_bytype(MCE_PROC, n));
}
