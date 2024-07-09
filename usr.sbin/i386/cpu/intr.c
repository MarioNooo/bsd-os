/*
 * Copyright (c) 1998 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	intr.c,v 1.21 2002/04/04 20:37:33 jch Exp
 */
#include "cpu.h"

#define	EDGE	0
#define	LEVEL	1
#define	HIGH	1
#define	LOW	0

char ovr_irq[] = "^irq" RNUM "=" RNUM "," RNUM "(,(level|excl|low|edge|high){1,1})*$";
char ovr_nopci[] = "^nopci$";
char ovr_noisa[] = "^noisa$";
char ovr_pccard[] = "^pccard$";

int using_extint;

struct ithdinfo *nit;		/* new interrupt setup */
struct ithdinfo *miscitp;

static int add_pci_sources __P((int irq, struct ithdinfo *itp));
static void add_isa_sources __P((int irq, struct ithdinfo *itp));
static int do_irq_ovr __P((int irq, struct ithdinfo *itp));
static void assign_idt __P((void));
static int idt_sort_func __P((const void *a, const void *b));

/*
 * Set up all interrupt routing
 */
void
setup_intr_routing(void)
{
	int irq;
	int new_thd_id = 100;
	struct ithdinfo *itp;
	struct ithdinfo *nitp;

	/* Show incoming interrupt tree */
	if (vflag > 2) {
		printf("Initial interrupt routing:\n"
		       "==========================\n");
		piroute(ihead);
	}

	/* Complain if BIOS wants to use a local APIC for I/O intrs */
	check_local_intrs();

	/*
	 * Create start of new tree by copying all threads and their
	 * associated handlers; this tree has no sources yet.
	 *
	 * Note that shadowing the handler chains in the new thread breaks
	 * LIST_INSERT_BEFORE(), this is OK since we never add handlers here.
	 */
	for (itp = ihead; itp != NULL; itp = itp->it_next) {
		/* Ignore soft interrupts */
		if (itp->it_flag & P_SITHD)
			continue;
		nitp = malloc(sizeof(*itp));
		bcopy(itp, nitp, sizeof(*itp));
		LIST_INIT(&nitp->it_isrchead);
		nitp->it_next = nit;
		nitp->it_excl = 0;
		itp->it_thd = (ithd_t *)nitp;	/* use kernel only field */
		nit = nitp;
	}

	/*
	 * Create a special procless thread descriptor for 
	 * sources that don't get a real thread.
	 */
	miscitp = newthd(&nit);
	miscitp->it_id = -1;
	strcpy(miscitp->it_name, "NoThdSrcs");
	miscitp->it_prio = 0xff;
	miscitp->it_excl = 0;

	/*
	 * Route special interrupts
	 */
	route_special(ApicNMI);
	route_special(ApicSMI);

	/*
	 * Generate sources for each IRQ
	 */
	if (vflag)
		printf("\nRouting H/W interrupts:\n");
	for (irq = 0; irq < 16; irq++) {
		if (vflag > 1)
			printf("\tIRQ %u:\n", irq);
		/*
		 * Get (new) thread for IRQ in question
		 */
		if ((itp = find_ithd(irq)) == NULL) {
			/* No thread, unused IRQ */
			switch (irq) {
			case 2:
				/* Chain interrupt */
				if (vflag > 1)
					printf("\t\tReserved (8259 chain)\n");
				continue;

			case 13:
				/* FP interrupt */
				if (vflag > 1)
					printf("\t\tReserved (FP co-proceesor)\n");
				continue;

			default:
				if (!ovr(ovr_pccard)) {
					if (vflag > 1)
						printf("\t\tUnused interrupt\n");
					continue;
				}
				break;
			}

			if (vflag > 1)
				printf("\t\tCreating new thread\n");
			/*
			 * If we have to deal with dynamically added PCMCIA
			 * cards, then create properly routed threads for all
			 * unused interrupts.
			 */
			itp = newthd(&nit);
			itp->it_id = new_thd_id++;
			snprintf(itp->it_name, sizeof (itp->it_name),
			    "Irq%d", irq);
			itp->it_prio = PI_DULL;
		}

		/* Clock might get special treatment */
		if (irq == 0 && route_clock())
			continue;

		/*
		 * If overrides were given, add the override sources only
		 */
		if (do_irq_ovr(irq, itp)) {
			if (vflag > 1)
				printf("\t\tUsing interrupt override\n");
			continue;
		}

		/*
		 * Add all PCI and ISA sources for this IRQ.  We only
		 * add ISA sources if there are no PCI sources.  Level
		 * triggered ISA sources are quite rare and we have
		 * seen some BIOSes that report level triggered ISA
		 * sources on an IRQ even though they are reporting
		 * the individual PCI sources.
		 */
		if (add_pci_sources(irq, itp) == 0)
			add_isa_sources(irq, itp);
	}

	/* Recalculate interrupt priorities given the whole picture */
	assign_idt();

	/* Sanity checks */
	intr_sanity();

	if (vflag > 2) {
		printf("\nNew interrupt routing tree:\n"
			 "===========================\n");
		piroute(nit);
	}

	/* Send to kernel */
	upload_itree(nit);
}

/*
 * Add all PCI sources for a given thread
 */
static int
add_pci_sources(int irq, struct ithdinfo *itp)
{
	struct mce_bus *busp;
	struct mce_io_intr *mip;
	int apic;
	int pin;
	int level;
	int pol;
	int flags;
	int i;
	int nsrcs;

	nsrcs = 0;

	if (vflag > 1)
		printf("\t\tPCI sources:\n");

	if (ovr(ovr_nopci)) {
		if (vflag > 1)
			printf("\t\t\tOverridden\n");
		return (nsrcs);
	}

	for (i = 0; ; i++) {
		if ((mip = get_pci_intr(i, &busp)) == NULL)
			break;
		if (irq != find_pci_irq(mip, busp))
			continue;
		apic = mip->mce_io_intr_dst_apic_id;
		pin = mip->mce_io_intr_dst_apic_int;
		pol = LOW;
		flags = mip->mce_io_intr_flags;
		if ((flags & MCE_INTR_FL_PO) == MCE_PO_HIGH)
			pol = HIGH;
		level = LEVEL;
		if ((flags & MCE_INTR_FL_EL) == MCE_FL_EDGE)
			level = EDGE;
		add_apic_normal(itp, apic, pin, level, pol, level == EDGE);
		nsrcs++;
	}

	return (nsrcs);
}

/*
 * Add all ISA sources for a given thread
 */
static void
add_isa_sources(int irq, struct ithdinfo *itp)
{
	struct mce_io_intr *mip;
	int i;
	int apic;
	int pin;
	int level;
	int pol;
	int flags;

	if (vflag > 1)
		printf("\t\tISA sources:\n");
	if (ovr(ovr_noisa)) {
		if (vflag > 1)
			printf("\t\t\tOverridden\n");
		return;
	}
	for (i = 0; ; i++) {
		if ((mip = get_isa_intr(i)) == NULL)
			break;
		if (irq != mip->mce_io_intr_src_bus_irq)
			continue;
		apic = mip->mce_io_intr_dst_apic_id;
		pin = mip->mce_io_intr_dst_apic_int;
		level = EDGE;
		flags = mip->mce_io_intr_flags;
		if ((flags & MCE_INTR_FL_EL) == MCE_FL_LEVEL ||
		    isa_lvlmode(itp)) {
			level = LEVEL;
			/* Assume its really PCI for 'confirms' */
			pol = LOW;
		} else
			pol = HIGH;
		if ((flags & MCE_INTR_FL_PO) == MCE_PO_HIGH)
			pol = HIGH;
		if ((flags & MCE_INTR_FL_PO) == MCE_PO_LOW)
			pol = LOW;
		add_apic_normal(itp, apic, pin, level, pol, level == EDGE);
	}
}

/*
 * Check for and route any overrides for this IRQ
 */
static int
do_irq_ovr(int irq, struct ithdinfo *itp)
{
	int n[8];
	int i;
	int rc;
	int level;
	int excl;
	int pol;
	char *o;

	rc = 0;
	for (i = 0; ; i++) {
		if ((o = ovrn(ovr_irq, i, n)) == NULL)
			return (rc);
		if (n[0] != irq)
			continue;
		rc = 1;
		level = -1;
		if (ovr_chk("level", o))
			level = LEVEL;
		if (ovr_chk("edge", o)) {
			if (level != -1)
				errx(1, "'edge' and 'level' are "
				    "mutually exclusive: %s", o);
			level = EDGE;
		}
		if (level == -1)
			level = EDGE;
		excl = ovr_chk("excl", o);
		pol = -1;
		if (ovr_chk("low", o))
			pol = LOW;
		if (ovr_chk("high", o)) {
			if (pol != -1)
				errx(1, "'high' and 'low' are "
				    "mutually exclusive: %s", o);
			pol = HIGH;
		}
		if (pol == -1)
			pol = HIGH;
		add_apic_normal(itp, n[1], n[2], level, pol, excl);
	}
}

/* Used for sort */
struct ss {
	int prio;
	isrc_t *src;
};

/*
 * Assign IDT entries based on thread priorities
 */
static void
assign_idt(void)
{
	isrc_t *isp;
	struct ithdinfo *itp;
	struct ss *nsp;
	struct ss *sp;
	int dm;
	int i;
	int ipprio;
	int pass;
	int prios;
	int srccnt;
	int vec;

	if (vflag)
		printf("\nCalculating interrupt priorities:\n");

	/*
	 * Count sources, then gather an array of sources and their priorities
	 */
	srccnt = 0;
	nsp = sp = NULL;		/* XXX Shut up GCC -Wall */
	for (pass = 0; pass < 2; pass++) {
		for (itp = nit; itp != NULL; itp = itp->it_next) {
			LIST_FOREACH(isp, &itp->it_isrchead, is_tlist) {
				switch (isp->is_type) {
				case ApicNMI:
				case ApicSMI:
				case LclVecTab:
					/* These don't get IDT entries */
					continue;
				default:
					break;
				}
				if (pass == 0)
					srccnt++;
				else {
					/*
					 * For threadless sources the IDT
					 * field gives a per-source priority
					 */
					nsp->prio = itp->it_prio;
					if (nsp->prio == 0xff)
						nsp->prio = isp->is_idt;
					nsp->src = isp;
					nsp++;
				}
			}
		}
		if (pass == 0)
			nsp = sp = malloc(sizeof(*sp) * srccnt);
	}

	/*
	 * Sort sources by priority (numerically highest to lowest)
	 */
	qsort(sp, srccnt, sizeof(*sp), idt_sort_func);
	prios = (IDT_MAX_IOVEC >> 4) - (IDT_MIN_IOVEC >> 4) + 1;
	ipprio = srccnt / prios;
	dm = srccnt % prios;
	if (dm != 0)
		ipprio++;
	/* 
	 * Before P4's only two interrupts could be queued per
	 * priority band (high nibble of IDT).  P4's allow two per
	 * vector so don't complain.
	 */
	if (ipprio > 2 && cpu_min_family < CPU_P4)
		warnx("Must allocate %d interrupts per priority"
		    "band (2 is max recommended)", ipprio);

	if (vflag > 1)
		printf("\t%u sources, %d per band\n", srccnt, ipprio);

	/*
	 * Now allocate them, lowest to highest priority
	 */
	i = ipprio;
	vec = IDT_MIN_IOVEC - 1;
	for (nsp = sp; nsp != &sp[srccnt]; nsp++) {
		if (i--)
			vec++;
		else {
			if (--dm == 0)
				ipprio--;
			i = ipprio - 1;
			/* next priority band */
			vec = (vec + 0x10) & ~0xf;
		}
		if (vec == LINUX_VEC)		/* skip Linux IDT */
			vec++;
		if (vec > IDT_MAX_IOVEC)
			errx(1, "IDT entry %d out of range (internal error)",
			    vec);
		isp = nsp->src;
		isp->is_idt = vec;
		switch (isp->is_type) {
			isrc_apic_t *iap;

		case ApicNormal:
		case ApicUnused:
		case ApicClk:
		case ApicClkHack:
			iap = (isrc_apic_t *)isp;
			iap->is_enable |= vec;
			iap->is_disable |= vec;
			break;
		default:
			errx(1, "assign_idt: bad type for IDT sort: %d\n",
			    isp->is_type);
		}
	}

	if (vflag > 1)
		printf("\tUsing vectors %#02x through %#02x\n",
		    IDT_MIN_IOVEC, vec);

	free(sp);
}

/*
 * Priority sort function, sorts lowest priority first
 */
static int
idt_sort_func(const void *a, const void *b)
{
	struct ss const *sa = a;
	struct ss const *sb = b;

	return (sb->prio - sa->prio);
}
