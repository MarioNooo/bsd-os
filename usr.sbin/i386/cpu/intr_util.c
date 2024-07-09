/*
 * Copyright (c) 1998 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	intr_util.c,v 1.6 2002/04/08 20:44:33 jch Exp
 */
#include "cpu.h"

/*
 * Find the thread that handles a given IRQ, this first finds the original
 * thread in the downloaded tree (headed at ihead) using the PIC sources
 * on these threads to determine which services the IRQ, then it returns
 * a pointer to the corresponding 'new' thread (in the 'nit' tree).
 */
struct ithdinfo *
find_ithd(int irq)
{
	struct ithdinfo *itp;
	isrc_t *isp;
	isrc_pic_t *ispp;
	struct ithdinfo *found;
	int tirq;

	if (irq == 0)
		return (miscitp);
	found = NULL;
	for (itp = ihead; itp != NULL; itp = itp->it_next) {
		/* Find IRQ, make sure all srcs agree */
		if (itp->it_flag & P_SITHD)
			continue;
		tirq = -1;
		LIST_FOREACH(isp, &itp->it_isrchead, is_tlist) {
			if (isp->is_type != PicNormal)
				errx(1, "find_ithd: type=%d internal err\n",
				    isp->is_type);
			ispp = (isrc_pic_t *)isp;
			if (tirq == -1)
				tirq = ispp->is_irq;
			else if (tirq != ispp->is_irq)
				errx(1, "different IRQs on same thd: %d %d",
				    tirq, ispp->is_irq);
		}
		if (tirq != irq)
			continue;
		if (found != NULL)
			errx(1, "find_ithd: multiple threads with IRQ %d", irq);
		found = itp;
	}
	/* get corresponding thread in new tree */
	if (found)
		found = (struct ithdinfo *)found->it_thd;
	return (found);
}

/*
 * Create a new interrupt thread on given list
 */
struct ithdinfo *
newthd(struct ithdinfo **hdp)
{
	struct ithdinfo *itp;

	itp = malloc(sizeof(*itp));
	LIST_INIT(&itp->it_ihhead);
	LIST_INIT(&itp->it_isrchead);
	itp->it_next = *hdp;
	*hdp = itp;
	return (itp);
}

/*
 * Set up an ApicNormal source on given thread
 */
void
add_apic_normal(struct ithdinfo *itp, int apic, int pin, int level, int pol,
    int excl)
{
	isrc_t *isp;
	isrc_apic_t *iap;
	char *level_s;
	char *pol_s;
	char *type_s;

	/* Check for multiple exclusive sources */
	if ((iap = (isrc_apic_t *)LIST_FIRST(&itp->it_isrchead)) != NULL &&
	    itp != miscitp && (excl || itp->it_excl))
		errx(1, "Trying to add src (apic=%d pin=%d) to "
		    "exclusive thd %s\n(apic=%d pin=%d already added)",
		    apic, pin, itp->it_name, iap->is_apic, iap->is_pin);

	/* Check for duplicate pin assignments */
	LIST_FOREACH(isp, &itp->it_isrchead, is_tlist) {
		iap = (isrc_apic_t *)isp;

		if (apic < iap->is_apic)
			continue;
		if (apic > iap->is_apic)
			break;
		/* Same APIC */
		if (pin < iap->is_pin)
			continue;
		if (pin > iap->is_pin)
			break;
		/* Same pin, check for a conflict */
		if (((level == LEVEL) != 
			((iap->is_disable & APIC_TM_LEVEL) == APIC_TM_LEVEL)) ||
		    ((pol == LOW) !=
			((iap->is_disable & APIC_PL_LOW) == APIC_PL_LOW)))
			errx(1,
			    "Apic %d pin %d has conflicting source attributes "
			    "for thread %s\n",
			    apic, pin, itp->it_name);
		/* Ignore duplicate source on this pin */
		return;
	}

	if (itp != miscitp) {
		type_s = "ApicNormal";
		iap = add_apic_src(itp, ApicNormal, apic, pin);
	} else {
		type_s = "ApicClock";
		iap = add_apic_src(itp, ApicClk, apic, pin);
	}
	iap->is_high = APIC_DF_BCAST;
	iap->is_disable = APIC_MS_DISABLE | APIC_DL_LOWPR;
	if (level == LEVEL) {
		level_s = "level";
		iap->is_src.is_flags |= IS_LEVEL;
		iap->is_disable |= APIC_TM_LEVEL;
	} else
		level_s = "edge";
	if (pol == LOW) {
		pol_s = "low";
		iap->is_disable |= APIC_PL_LOW;
	} else
		pol_s = "high";
	iap->is_enable = iap->is_disable & ~APIC_MS_DISABLE;
	itp->it_excl = excl;
	if (vflag > 1) {
		printf("\t\t\t%s %u.%u %s %s %s\n",
		    type_s, apic, pin, level_s, pol_s, excl ? "exclusive" : "");
	}
}

/*
 * Add an APIC source to a thread
 */
isrc_apic_t *
add_apic_src(struct ithdinfo *itp, isrc_type type, int apic, int pin)
{
	isrc_apic_t *iap;
	isrc_t *isp;

	iap = calloc(1, sizeof(*iap));
	iap->is_src.is_type = type;
	iap->is_src.is_idt = PI_REALTIME;	/* Default to highest prio */
	iap->is_apic = apic;
	iap->is_pin = pin;
	iap->is_base = (int *)IO_APIC_VA(apic);
	iap->is_regsel = APIC_IO_REDIR_LOW(pin);

	/* Insert source ordered by APIC and pin */
	if (LIST_EMPTY(&itp->it_isrchead))
		LIST_INSERT_HEAD(&itp->it_isrchead, (isrc_t *)iap, is_tlist);
	else
		LIST_FOREACH(isp, &itp->it_isrchead, is_tlist) {
			isrc_apic_t *iap2 = (isrc_apic_t *)isp;

			if (apic < iap2->is_apic ||
			    (apic == iap2->is_apic && pin < iap2->is_pin)) {
				LIST_INSERT_BEFORE(isp, (isrc_t *)iap, is_tlist);
				break;
			}
			if (LIST_NEXT(isp, is_tlist) == LIST_END(&itp->it_isrchead)) {
				LIST_INSERT_AFTER(isp, (isrc_t *)iap, is_tlist);
				break;
			}
		}

	return (iap);
}

/*
 * See if a given IRQ is level triggered:
 *	- if it was level triggered from Pic mode (probably a PCI IRQ)
 *	- if any driver on IRQ requested level mode
 */
int
isa_lvlmode(struct ithdinfo *itp)
{
	isrc_t *isp;
	ihand_t *ihp;
	int lvl = 0;

	/* See if any sources were level triggered from Pic mode */
	LIST_FOREACH(isp, &itp->it_isrchead, is_tlist)
		lvl |= isp->is_flags & IS_LEVEL;

	/* See if any drivers want level mode */ 
	LIST_FOREACH(ihp, &itp->it_ihhead, ih_list)
		lvl |= ihp->ih_flags & IH_APICLEVEL;

	return (lvl);
}

/*
 * Find the IRQ number a PCI interrupt line should be attached to
 */
int
find_pci_irq(struct mce_io_intr *ip, struct mce_bus *busp)
{
	int agent;
	int pin;
	int irq;
	int i;
	int pci_busses;
	struct mce_bus *tbus;
	struct pcidata *pip;

	/*
	 * Count the PCI busses
	 * 
	 * This is used by find_pci_irq, if there is only one bus we assume
	 * that it may actually be bus ID 0 (instead of whatever is actually
	 * reported by the BIOS, usually 1).
	 *
	 * Its a little wasteful to do this each time, but code space matters
	 * more in this case than speed.
	 */
	pci_busses = 0;
	for (i = 0; (tbus = get_ent_bytype(MCE_BUS, i)) != NULL; i++)
		if (strncmp(tbus->mce_bus_type, "PCI   ", 6) == 0)
			pci_busses++;

	agent = ip->mce_io_intr_src_bus_irq >> 2;
	pin = (ip->mce_io_intr_src_bus_irq & 0x3) + 1;
	irq = -1;
	for (pip = phead; pip != NULL; pip = pip->next) {
		if (pip->pi.cpc_bus != busp->mce_bus_id &&
		    (pip->pi.cpc_bus != 0 || pci_busses != 1))
			continue;
		if (pip->pi.cpc_agent != agent)
			continue;
		if (pip->pi.cpc_pin != pin)
			continue;
		irq = pip->pi.cpc_irq;
		break;
	}
	return (irq);
}

/*
 * Complain if any normal interrupts routed to a local APIC
 */
void
check_local_intrs(void)
{
	struct mce_io_intr *ip;
	int i;

	if (vflag)
		printf("\nChecking that all I/O interrupts are to I/O APICs:\n");

	for (i = 0; (ip = get_intr(i)) != NULL; i++) {
		if (ip->mce_io_intr_type != MCE_INTR_TYPE_INT)
			continue;
		if (VCPU(ip->mce_io_intr_dst_apic_id))
			warnx("BIOS routes a normal interrupt (bus=%d irq=%x)"
			    " to a cpu local APIC (ignored)",
			    ip->mce_io_intr_src_bus_id,
			    ip->mce_io_intr_src_bus_irq);
	}
}

/*
 * Send new interrupt tree to kernel
 */
void
upload_itree(struct ithdinfo *ihd)
{
	struct ithdinfo *itp;
	isrc_t *isp;
	ihand_t *ihp;
	int n[5];
	int sz;

	if (debug)
		return;

	if (vflag)
		printf("\nLoading new tree into kernel\n");

	for (itp = ihd; itp != NULL; itp = itp->it_next) {
		/* Tell kernel about the thread */
		n[0] = CTL_MACHDEP;
		n[1] = CPU_SIOINTR;
		n[2] = CPUINTR_THREAD;
		n[3] = itp->it_id;
		if (sysctl(n, 4, NULL, NULL, itp, sizeof(*itp)) < 0)
			err(1, "sysctl sio thread");

		/* Send up all sources */
		n[2] = CPUINTR_SRC;
		LIST_FOREACH(isp, &itp->it_isrchead, is_tlist) {
			sz = ISSIZE(isp->is_type);
			if (sysctl(n, 4, NULL, NULL, isp, sz) < 0)
				err(1, "sysctl sio src");
		}

		/* Send up handlers */
		n[2] = CPUINTR_HAND;
		LIST_FOREACH(ihp, &itp->it_ihhead, ih_list)
			if (sysctl(n, 4, NULL, NULL, ihp, sizeof(*ihp)) < 0)
				err(1, "sysctl sio hand");
	}
}
