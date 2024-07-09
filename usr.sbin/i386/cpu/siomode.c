/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	siomode.c,v 1.7 2003/09/16 13:16:03 jch Exp
 */
#include "cpu.h"

char ovr_notpr[] = "^notpr$";

/*
 * Enter Symmetric I/O mode (switch to using APIC's, set up for MP mode)
 */
int
siomode(int ac, char **av)
{
	if (miscinfo.siomode != 0)
		errx(1, "Already in symmetric I/O mode");
	find_valid_cpus();
	return (do_siomode());
}

/*
 * The guts of turning on symmetric I/O mode
 */
int
do_siomode(void)
{
	u_long paddr;
	int n[3];
	int v[2];

	memset(v, 0, sizeof(v));

	if (!mp_capable())
		errx(1, "BIOS reports machine is not MP capable");

	if (vflag)
		printf("Switching to symmetric I/O mode\n");

	/* Initialize the bootstrap processor (start idle proc, set ID) */
	if (!debug) {
		/*
		 * First map the local APIC for the boot processor. As far
		 * as the kernel is concerned we are mapping CPU-0, but we
		 * must map at the location of the boot processor since
		 * Cpu-0 will be renamed to the boot processor.
		 */
		v[0] = get_boot_processor();
		n[0] = CTL_MACHDEP;
		n[1] = CPU_LAPIC;
		n[2] = 0;
		paddr = get_local_apic_addr(v[0]);
		if (vflag)
			printf("Map boot CPU local APIC: boot-cpu=%d "
			    "addr=%lx\n", get_boot_processor(), paddr);
		if (sysctl(n, 3, NULL, NULL, &paddr, sizeof(paddr)) < 0)
			err(1, "CPU_LAPIC for boot processor");

		/*
		 * Finally, rename the boot processor to its real APIC ID.
		 */
		n[1] = CPU_BOOTCPU;
		if (sysctl(n, 2, NULL, NULL, v, sizeof(v[0])) < 0)
			err(1, "Can't initialize boot processor");
	}

	/* Map the rest of the APICs into kernel memory */
	map_lapics();
	map_ioapics();

	/* Program interrupt routing */
	setup_intr_routing();

	/*
	 * Decide if we need to ask kernel to do IMCR algorithm (used to turn
	 * off PIC compatibility mode on machines that don't do virtual
	 * wire mode).
	 */
	if (check_for_imcr())
		v[1] |= CPU_IMCR;

	/*
	 * Some broken chipsets will not deliver interrupts if the TPR
	 * registers are set to anything other than zero.  Signal the
	 * kernel to map all priorities to zero.
	 */
	if (ovr(ovr_notpr))
		v[1] |= CPU_NOTPR;

	/*
	 * Finally, we're ready to actually turn of the PIC's and fire up
	 * the APIC's in native symmetric I/O mode.
	 */
	if (vflag && v[1]) {
		printf("Flags for symmetric I/O mode:\n");
		if (v[1] & CPU_IMCR)
			printf("\tWill access IMCR register to switch modes\n");
		if (v[1] & CPU_NOSTOPBSP)
			printf("\tBoot processor will not be allowed to "
			    "stop\n");
		if (v[1] & CPU_NOTPR)
			printf("\tDisabling the use of Task Priority "
			    "Register (TPR)\n");
	}
	if (debug)
		return (0);

	if (vflag)
		printf("\nSwitching to SIOMODE\n");
	n[0] = CTL_MACHDEP;
	n[1] = CPU_SIOMODE;
	v[0] = 1;
	if (sysctl(n, 2, NULL, NULL, v, sizeof(v)) < 0)
		err(1, "Failed to switch to symmetric I/O mode");
	return (0);
}

