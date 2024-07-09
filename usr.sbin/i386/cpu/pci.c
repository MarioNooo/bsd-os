/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	pci.c,v 1.3 2001/11/05 23:06:42 polk Exp
 */
#include "cpu.h"

/*
 * Display PCI device info
 */
int
disp_pci(int ac, char **av)
{
	struct pcidata *pp;

	printf("Bus Agent Func Vendor Device Irq Pin\n"
	       "=== ===== ==== ====== ====== === ===\n");
	for (pp = phead; pp != NULL; pp = pp->next) {
		printf("%3x %5x %4x %6x %6x %3x %c\n",
		    pp->pi.cpc_bus, pp->pi.cpc_agent, pp->pi.cpc_function,
		    pp->pi.cpc_vendor, pp->pi.cpc_device, pp->pi.cpc_irq,
		    "XABCD"[pp->pi.cpc_pin]);
	}
	return (0);
}

/*
 * See if this chipset requires the 'clock hack'
 */
int
check_clkhack(void)
{
	struct pcidata *pp;

	for (pp = phead; pp != NULL; pp = pp->next) {
		if (pp->pi.cpc_bus == 0 &&
		    pp->pi.cpc_vendor == 0x8086 &&
		    (pp->pi.cpc_device == 0x7000 || 
			pp->pi.cpc_device == 0x7110) &&
		    pp->pi.cpc_function == 0) {
			if (vflag)
				warnx("82371 (Triton II/PIIX3 or PIIX4) "
				    "clock interrupt workaround indicated");
			return (1);
		}
	}
	return (0);
}
