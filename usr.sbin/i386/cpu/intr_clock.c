/*
 * Copyright (c) 1998 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	intr_clock.c,v 1.3 2001/11/06 02:53:52 jch Exp
 */
#include "cpu.h"

char ovr_noextclk[] = "^noextclk$";
char ovr_extclk[] = "^extclk$";
char ovr_extdest[] = "^extdest=" RNUM "$";
char ovr_extint[] = "^extint=" RNUM "," RNUM "$";
char ovr_noext[] = "^noext$";

static int get_extint __P((int *apic, int *pin));

/*
 * See if we must route the clock via the PIIX3 hack, returns 0 if not
 * routed this way.
 */
int
route_clock(void)
{
	isrc_apic_t *isp;
	int apic;		/* Apic/pin for selected ExtINT input */
	int pin;

	if (ovr(ovr_noextclk) && ovr(ovr_extclk))
		errx(1, "Can't specify both \"extclk\" and \"noextclk\"");

	if (get_extint(&apic, &pin) == 0)
		return (0);

	/*
	 * Determine if 82371 clock hack is called for. This hack sets up the
	 * ExtINT pin on the I/O APIC to run as a normal interrupt (Lowest
	 * Priority delivery mode) instead of as an ExtINT. This will only
	 * work if only one interrupt source to the 8259 exists (and it must
	 * be the clock the way the handlers are currently written) .
	 *
	 * We can't tell directly that this is set up this way, some BIOS's
	 * lie and tell us the clock is connected to a different APIC pin.
	 * We do the hack if overridden or on an evil chipset and the
	 * ExtINT is on an I/O APIC pin.
	 */
	if (get_io_apic(apic) != NULL &&
	    (check_clkhack() || ovr(ovr_extclk)) &&
	    !ovr(ovr_noextclk)) {
		isp = add_apic_src(miscitp, ApicClkHack, apic, pin);
		isp->is_high = APIC_DF_BCAST;
		isp->is_enable = APIC_DL_LOWPR | APIC_TM_EDGE | APIC_PL_HIGH;
		if (vflag > 1)
			printf("\t\tApicClkHack %d.%d edge high\n", apic, pin);
		return (1);
	}
	return (0);
}

/*
 * Determine 'best' ExtINT input (apic/pin)
 */
static int
get_extint(int *apic, int *pin)
{
	int ext_apic;
	int ext_pin;
	int n[8];
	int no_bsp_restrict;
	int bsp;
	char *o;
	struct mce_io_intr *idp;
	struct mce_lc_intr *ldp;
	int i;

	if (ovr(ovr_noext)) {
		if ((ovr(ovr_extint) || ovr(ovr_extclk) || ovr(ovr_extdest)))
			errx(1, "Can't specify other extint overrides "
			    "with \"noext\"");
		/* User says don't do anything */
		return (0);
	}
	if (ovrn(ovr_extint, 1, n))
		errx(1, "Only one \"extint\" override allowed");

	if (ovrn(ovr_extdest, 1, n))
		errx(1, "Only one \"extdest\" override allowed");

	/* If there are no ExtINT connections, we have nothing to do */
	if (!ovr(ovr_extint) && get_apic_extint(0) == NULL &&
	    get_lcl_extint(0) == NULL) {
		if (ovrn(ovr_extclk, 0, n))
			errx(1, "ovr_extclk: No ExtINT source defined");
		return (0);
	}

	/*
	 * Select which ExtINT input to use, prefer the I/O APIC pins over
	 * direct CPU inputs.
	 */
	no_bsp_restrict = 0;
	bsp = get_boot_processor();
	if ((o = ovrn(ovr_extint, 0, n))) {
		/*
		 * See if user is telling us how to do it
		 */
		ext_apic = n[0];
		ext_pin = n[1];
		if (VCPU(ext_apic)) {
			if (ext_apic != bsp) {
				warnx("%s: routes ExtINT to non boot processor",
				    o);
				no_bsp_restrict = 1;
			}
		} else {
			if (get_io_apic(ext_apic) == NULL)
				errx(1, "%s: Invalid I/O APIC: %x", o,
				    ext_apic);
		}
	} else {
		/*
		 * Try to figure it out ourselves
		 */
		if ((idp = get_apic_extint(0)) != NULL) {
			ext_apic = idp->mce_io_intr_dst_apic_id;
			ext_pin = idp->mce_io_intr_dst_apic_int;
			if (vflag && get_apic_extint(1))
				warnx("BIOS defines multiple ExtINT APIC "
				    "sources, using first one");
		} else {
			if (get_lcl_extint(1)) {
				/*
				 * Multiple CPU's are wired to ExtINT
				 * (we assume they are all on the same ExtINT).
				 * Pick the boot processor: if its not
				 * connected we fail here. It is hard to
				 * support ExtINT on an AP since the AP
				 * isn't up yet, so sending interrupts to it
				 * would be difficult. This could be worked
				 * around with some effort, but it doesn't
				 * seem worth it at this point.
				 */
				for (i = 0; (ldp = get_lcl_extint(i)) != NULL;
				    i++) {
					ext_apic = ldp->mce_lc_intr_dst_apic_id;
					if (ext_apic == bsp)
						break;
					if (ext_apic == 0xff) {
						ext_apic = bsp;
						break;
					}
				}
				if (ldp == NULL)
					errx(1, "ExtINT is not connected to "
					    "the boot processor");
				ext_pin = ldp->mce_lc_intr_dst_apic_int;
			} else {
				ldp = get_lcl_extint(0);
				ext_apic = ldp->mce_lc_intr_dst_apic_id;
				ext_pin = ldp->mce_lc_intr_dst_apic_int;
				if (ext_apic == 0xff)
				    ext_apic = bsp;
				if (ext_apic != bsp)
					errx(1, "ExtINT is not connected to "
					    "the boot processor");
			}
		}
	}
	*apic = ext_apic;
	*pin = ext_pin;
	return (1);
}
