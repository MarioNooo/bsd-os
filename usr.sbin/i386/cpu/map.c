/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	map.c,v 1.5 2001/11/05 23:06:42 polk Exp
 */
#include "cpu.h"

static struct apic_entry apic_table[MAX_IOAPIC];

static u_int apic_fixed2log[MAX_IOAPIC];

static char *apic_types[] = {"Unused", "Local", "I/O" };

/*
 * Map all known local apics
 */
void
map_lapics(void)
{
	struct apic_entry *apic;
	u_long addr;
	int n[3];

	n[0] = CTL_MACHDEP;
	n[1] = CPU_LAPIC;

	if (vflag)
		printf("\nMapping local (CPU) APICs:\n");

	/* Map all local APIC physical addresses */
	for (apic = apic_table; apic < &apic_table[MAXCPUS]; apic++) {
		u_int apic_id = apic - apic_table;

		if ((addr = get_local_apic_addr(apic_id)) == 0)
			continue;
		if (apic->apic_type != APIC_TYPE_UNUSED)
			err(1, 
			    "local APIC ID %u at %lx already in use by %s APIC at %lx",
			    apic_id, addr,
			    apic->apic_type == APIC_TYPE_LOCAL ?
			        "local" : "I/O",
			    apic->apic_paddr);
		apic->apic_type = APIC_TYPE_LOCAL;
		apic->apic_origid = apic->apic_id = apic_id;
		apic->apic_paddr = addr;
		apic_fixed2log[apic_id] = apic_id;
		if (vflag > 1)
			printf("\tMap LCL APIC ID %u at %#lx\n", apic_id, addr);
		n[2] = apic_id;
		if (!debug &&
		    sysctl(n, 3, NULL, NULL, &addr, sizeof(addr)) < 0)
			err(1, "map local apic cpu=0x%x addr=0x%x", 
			    apic_id, addr);
	}
}

/*
 * Map all known I/O APICs
 */
void
map_ioapics(void)
{
	struct cpu_phmap cpm;
	struct mce_io_apic *iap;
	struct apic_entry remap_table[MAXCPUS];
	struct apic_entry *apic, *remap;
	int n[3];

	n[0] = CTL_MACHDEP;
	n[1] = CPU_IOAPIC;

	if (vflag)
		printf("\nMapping I/O APICs:\n");

	/* Discover all I/O APICs */
	memset(remap_table, 0, sizeof(remap_table));
	for (remap = remap_table; remap < &remap_table[MAX_IOAPIC]; remap++) {
		u_int apic_id = remap - remap_table;

		if ((iap = get_io_apic(apic_id)) == NULL)
			continue;
		if (remap->apic_type != APIC_TYPE_UNUSED)
			err(1,
			    "I/O APIC ID %u at %lx already in use by I/O APIC at %lx",
			    apic_id,
			    iap->mce_io_apic_addr,
			    remap->apic_paddr);
		remap->apic_type = APIC_TYPE_IO;
		remap->apic_origid = remap->apic_id = apic_id;
		remap->apic_paddr = (u_int32_t)iap->mce_io_apic_addr;
	}

	/* Do any necessary remapping */
	for (remap = remap_table; remap < &remap_table[MAX_IOAPIC]; remap++) {
		if (remap->apic_type != APIC_TYPE_IO)
			continue;
		/* Simple, default APIC ID not in use */
		apic = &apic_table[remap->apic_id];
		if (apic->apic_type == APIC_TYPE_UNUSED) {
			*apic = *remap;
			continue;
		}
#ifdef	notdef
		/* Search for a new slot */
		for (apic = apic_table; apic < &apic_table[MAX_IOAPIC];
		     apic++) {
			if (apic->apic_type != APIC_TYPE_UNUSED ||
			    remap_table[apic->apic_id].apic_type != 
			    APIC_TYPE_UNUSED)
				continue;
			*apic = *remap;
			apic->apic_id = apic - apic_table;
			break;
		}
#else
		errx(1, "%s APIC %u address conflicts with %s APIC",
		    apic_types[remap->apic_type], remap->apic_id,
		    apic_types[apic->apic_type]);
		    
#endif
		/* Uh oh! */
		if (apic == &apic_table[MAX_IOAPIC])
			errx(1, "Too many APICs");
	}

	/* Map all I/O apics */
	for (apic = apic_table; apic < &apic_table[MAX_IOAPIC]; apic++) {

		if (apic->apic_type != APIC_TYPE_IO)
			continue;

		/* Set logical to fixed mapping */
		apic_fixed2log[apic->apic_origid] = apic->apic_id;

		/* Do address space mapping */
		cpm.cph_id = apic->apic_id;
		cpm.cph_paddr = apic->apic_paddr;

		if (vflag > 1)
			printf("\tMap I/O APIC ID %u at %#lx\n", 
			    cpm.cph_id, cpm.cph_paddr);

		n[2] = apic->apic_origid;
		if (!debug &&
		    sysctl(n, 3, NULL, NULL, &cpm, sizeof(cpm)) < 0)
			err(1, "map io apic 0x%x addr=0x%x\n", 
			    cpm.cph_id, cpm.cph_paddr);
	}
}

/*
 * Given an APIC ID from the MP info, return the mapped ID.
 */
int
map_apic_id(int apic_id)
{
	struct apic_entry *apic;
	int log_id;

	if (apic_id < 0 || apic_id > MAX_IOAPIC)
		errx(1, "Invalid APIC specification: %x", apic_id);
	if ((log_id = apic_fixed2log[apic_id]) == -1)
		errx(1, "Unknown APIC: %x", apic_id);
	apic = &apic_table[log_id];

	if (apic->apic_type != APIC_TYPE_IO) {
		warnx("APIC ID %u, expected I/O APIC, found %s APIC",
		    apic_id,
		    apic_types[apic->apic_type]);
		return (NULL);
	}

	return (apic->apic_id);
}

