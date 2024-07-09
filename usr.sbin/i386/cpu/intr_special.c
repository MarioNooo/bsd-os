/*
 * Copyright (c) 1998 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	intr_special.c,v 1.4 2002/04/04 05:12:14 jch Exp
 */
#include "cpu.h"

char ovr_nmi[] = "^nmi=" RNUM "," RNUM "$";
char ovr_nmidest[] = "^nmidest=" RNUM "$";
char ovr_smi[] = "^smi=" RNUM "," RNUM "$";
char ovr_smidest[] = "^smidest=" RNUM "$";
char ovr_nonmi[] = "^nonmi$";
char ovr_nosmi[] = "^nosmi$";

static isrc_lvt_t *get_lvt_src __P((int cpu));
static void setup_special __P((isrc_type type, int apic, int pin, int dest, char *dbg));

/*
 * Route NMI or SMI sources
 */
void
route_special(isrc_type type)
{
	int n[16];
	struct mce_lc_intr *lp;
	struct mce_io_intr *ip;
	char *p;
	int dest;
	int i;
	int iosrc;
	int lsrc;
	int apic;
	int pin;
	char *dsc;
	char *udsc;
	char *ovr_dest;
	char *ovr_type;
	char *ovr_no;
	int mce_type;

	if (type == ApicNMI) {
		dsc = "nmi";
		udsc = "NMI";
		ovr_dest = ovr_nmidest;
		ovr_type = ovr_nmi;
		ovr_no = ovr_nonmi;
		mce_type = MCE_INTR_TYPE_NMI;
	} else {
		dsc = "smi";
		udsc = "SMI";
		ovr_dest = ovr_smidest;
		ovr_type = ovr_smi;
		ovr_no = ovr_nosmi;
		mce_type = MCE_INTR_TYPE_SMI;
	}
	
	if (vflag)
		printf("\nRouting %s interrupts:\n", udsc);

	/* Sanity checks on our overrides */
	if (ovr(ovr_no)) {
		if (ovr(ovr_dest) || ovr(ovr_type))
			errx(1, "no%s and %s or %sdest are mutually "
			    "exclusive", dsc, dsc, dsc);
		return;
	}
	if (ovrn(ovr_dest, 1, NULL))
		errx(1, "only one %sdest override allowed", dsc);

	/* Apply destination override */
	if ((p = ovrn(ovr_dest, 0, n)) != NULL) {
		dest = n[0] & 0xf;
		if (!VCPU(dest))
			errx(1, "Invalid %s destination: %s", udsc, p);
	} else 
		dest = 0xf;

	/*
	 * If any overrides given, we use them instead of the BIOS info
	 */
	if (ovr(ovr_type)) {
		for (i = 0; ; i++) {
			if ((p = ovrn(ovr_type, i, n)) == NULL)
				break;
			setup_special(type, n[0], n[1], dest, p);
		}
		return;
	}

	/*
	 * Count BIOS entry types
	 */
	iosrc = 0;
	lsrc = 0;
	apic = -1;		/* XXX compiler warnings */
	pin = 0;
	for (i = 0; ; i++) {
		if ((lp = get_local_intr(i)) == NULL)
			break;
		if (lp->mce_lc_intr_type != mce_type)
			continue;
		if (VCPU(lp->mce_lc_intr_dst_apic_id) ||
		    lp->mce_lc_intr_dst_apic_id == 0xff)
			lsrc++;
	}
	for (i = 0; ; i++) {
		if ((ip = get_intr(i)) == NULL)
			break;
		if (ip->mce_io_intr_type != mce_type)
			continue;
		if (iosrc++ == 0) {
			apic = ip->mce_io_intr_dst_apic_id;
			pin = ip->mce_io_intr_dst_apic_int;
		}
	}

	if (iosrc > 1)
		warnx("Multiple %s sources on I/O APIC, using first one "
		     "(%x/%x)", udsc, apic, pin);

	if (lsrc != 0) {
		if (iosrc != 0)
			warnx("Using %s sources on local CPU input pins, "
			    "ignoring I/O APIC %ss", udsc, udsc);
		for (i = 0; ; i++) {
			if ((lp = get_local_intr(i)) == NULL)
				break;
			if (lp->mce_lc_intr_type != mce_type)
				continue;
			if (lp->mce_lc_intr_dst_apic_id == 0xff) {
				for (apic = 0; apic < MAXCPUS; apic++) {
					if (!VCPU(apic))
						continue;
					setup_special(type, apic,
					    lp->mce_lc_intr_dst_apic_int, 0,
					    "BIOS LVT BCAST NMI/SMI");
				}
				continue;
			}
			if (!VCPU(lp->mce_lc_intr_dst_apic_id))
				continue;
			setup_special(type, lp->mce_lc_intr_dst_apic_id,
			    lp->mce_lc_intr_dst_apic_int, dest,
			    "BIOS LVT NMI/SMI");
		}
	} else if (iosrc != 0)
		/* Set it up on the I/O APIC */
		setup_special(type, apic, pin, dest, "BIOS I/O NMI/SMI");
		
	if (vflag && iosrc == 0 && lsrc == 0)
		warnx("Warning: No %s sources defined by BIOS", udsc);
}

/*
 * Set up an NMI or SMI source
 */
static void
setup_special(isrc_type type, int apic, int pin, int dest, char *dbg)
{
	isrc_apic_t *isp;
	isrc_lvt_t *ilp;
	int dlmode;
	char *udsc;
	char *type_s;

	if (type == ApicNMI) {
		udsc = "NMI";
		dlmode = APIC_DL_NMI;
	} else {
		udsc = "SMI";
		dlmode = APIC_DL_SMI;
	}

	if (apic < 0 || apic >= MAXCPUS)
		errx(1, "Invalid apic number: %s", dbg);
	if (VCPU(apic)) {
		/* It's a CPU */
		type_s = "CPU";
		if (pin < 0 || pin > 1)
			errx(1, "Invalid CPU pin #: %s", dbg);
		ilp = get_lvt_src(apic);
		if (ilp->is_lvt[pin] != APIC_MS_DISABLE)
			errx(1, "Cpu LVT pin #%d in use: %s", pin, dbg);
		ilp->is_lvt[pin] = dlmode;
	} else {
		/* It might be an I/O apic */
		type_s = "I/O";
		if (get_io_apic(apic) == NULL)
			errx(1, "Invalid APIC ID: %s", dbg);
		if (pin < 0 || pin > 23)	/* XXX */
			errx(1, "Invalid APIC pin #: %s", dbg);

		isp = add_apic_src(miscitp, type, apic, pin);
		isp->is_high = dest << APIC_DF_SHFT;
		isp->is_enable = dlmode;
	}

	if (vflag > 1)
		printf("\t%s %u.%u\n",
		    type_s, apic, pin);
}

/*
 * Find/create and LVT source (attach to misc thread)
 */
static isrc_lvt_t *
get_lvt_src(int cpu)
{
	isrc_t *isp;
	isrc_lvt_t *ilp;

	LIST_FOREACH(isp, &miscitp->it_isrchead, is_tlist) {
		ilp = (isrc_lvt_t *)isp;
		if (ilp->is_src.is_type != LclVecTab)
			continue;
		if (ilp->is_cpuno == cpu)
			return (ilp);
	}
	ilp = malloc(sizeof(*ilp));
	bzero(ilp, sizeof(*ilp));
	LIST_INSERT_HEAD(&miscitp->it_isrchead, (isrc_t *)ilp, is_tlist);
	ilp->is_src.is_type = LclVecTab;
	ilp->is_cpuno = cpu;
	ilp->is_lvt[0] = APIC_MS_DISABLE;
	ilp->is_lvt[1] = APIC_MS_DISABLE;
	return (ilp);
}
