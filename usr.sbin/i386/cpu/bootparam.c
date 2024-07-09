/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	bootparam.c,v 1.10 2001/11/05 23:06:41 polk Exp
 */
#include "cpu.h"

/*
 * Display boot parameters
 */
int
bootparam(int ac, char **av)
{

	pmp(fpsp);

	return (0);
}

static int
mps_cksum(void *vp, int len)
{
	u_char *p = vp, res = 0;

	while (len--) 
		res += *p++;

	return (res);
}

void
pmp(struct fps *f)
{
	struct mct *m;
	union mc {
		struct mp_cfg_entry ce;
		struct mce_proc ceproc;
		struct mce_bus cebus;
		struct mce_io_apic ceio;
		struct mce_io_intr ceioi;
		struct mce_lc_intr celci;
	} *mc;
#define mce	mc->ce
#define mceproc	mc->ceproc
#define mcebus	mc->cebus
#define mceio	mc->ceio
#define mceioi	mc->ceioi
#define mcelci	mc->celci
	int len;
	int i;
	u_long t;
	char *at;
	union ec {
		struct ece_map emap;
		struct ece_bus ebus;
		struct ece_compat ecompat;
	} *ec;
#define	emap	ec->emap
#define	ebus	ec->ebus
#define	ecompat	ec->ecompat

	printf("B_INTEL_MP\n");
	printf("   Floating Pointer Structure\n");
	printf("\tPhysical addr     : 0x%8.8x\n", (u_int)f->fps_cfg);
	printf("\tFPS length        : %d\n", f->fps_len);
	printf("\tRevision          : %d\n", f->fps_rev);
	printf("\tChecksum          : 0x%x\n", f->fps_chks);
	printf("\tConfig type       : %d\n", f->fps_cfgtype);
	printf("\tFeature bytes 2-5 :");
	phex(f->fps_feature, 4);
	if (f->fps_feature[0] & FPS_IMCRP)
		printf("\t\t[IMCRP]");
	printf("\n");
	if (f->fps_cfg == 0)
		return;
	printf("   MP base configuration table\n");
	m = (struct mct *)(f + 1);
	printf("\tRevision          : %d\n", m->mct_rev);
	printf("\tChecksum          : 0x%x\n", m->mct_chks);
	printf("\tOEM ID            : %-8.8s\n", m->mct_oem_id);
	printf("\tProduct ID        : %-12.12s\n",
	    m->mct_prod_id);
	printf("\tOEM table pointer : 0x%8.8x\n",
	    (u_int)m->mct_oem_table);
	printf("\tOEM table size    : %d\n",
	    m->mct_oem_table_size);
	printf("\tEntry count       : %d\n", m->mct_entry_cnt);
	printf("\tLocal APIC addr   : 0x%8.8x\n",
	    (u_int)m->mct_local_apic);
	printf("\tExtended table len: %d\n",
	    m->mct_ext_tab_len);
	printf("\tExt table checksum: 0x%x\n",
	    m->mct_ext_tab_chks);
	mc = (union mc *)(m + 1);
	i = 0;
	printf("   Base entries:\n");
	while (i++ < m->mct_entry_cnt) {
		printf("   ===== ");
		switch (mce.mce_entry_type) {
		case MCE_PROC:
			printf("Processor\n");
			printf("\tAPIC ID           : %d\n",
			    mceproc.mce_proc_local_apic_id);
			printf("\tAPIC version      : %d\n",
			    mceproc.mce_proc_local_apic_vers);
			printf("\tCPU flags         : 0x%x",
			    mceproc.mce_proc_cpu_flags);
			if (mceproc.mce_proc_cpu_flags & MCE_PROC_FL_EN)
				printf(" enabled");
			if (mceproc.mce_proc_cpu_flags & MCE_PROC_FL_BP)
				printf(" bootstrap");
			printf("\n");
			t = mceproc.mce_proc_cpu_signature;
			printf("\tSignature         : 0x%lx\n", t);
			printf("\t  Family          : %ld\n", t >> 8);
			printf("\t  Model           : %ld\n", (t >> 4) & 0xf);
			printf("\t  Stepping        : %ld\n", t & 0xf);
			printf("\tFeatures          : 0x%x\n",
			    mceproc.mce_proc_cpu_feature);
			len = MCE_PROC_LEN;
			break;
		case MCE_BUS:
			printf("Bus\n");
			printf("\tBus ID            : %d\n", mcebus.mce_bus_id);
			printf("\tBus Type          : %-6.6s\n",
			    mcebus.mce_bus_type);
			len = MCE_BUS_LEN;
			break;
		case MCE_IO_APIC:
			printf("IO Apic\n");
			printf("\tAPIC ID            : %d\n",
			    mceio.mce_io_apic_id);
			printf("\tAPIC version       : %d\n",
			    mceio.mce_io_apic_vers);
			printf("\tAPIC flags         : 0x%x",
			    mceio.mce_io_apic_flags);
			if (mceio.mce_io_apic_flags & MCE_IO_APIC_FL_EN)
				printf(" enabled");
			printf("\n");
			printf("\tAPIC address       : 0x%8.8x\n",
			    (u_int)mceio.mce_io_apic_addr);
			len = MCE_IO_APIC_LEN;
			break;
		case MCE_IO_INTR:
		case MCE_LC_INTR:
			if (mce.mce_entry_type == MCE_IO_INTR)
				at = "I/O";
			else
				at = "Lcl";
			printf("%s Interrupt\n", at);
			t = mceioi.mce_io_intr_type;
			printf("\tType              : ");
			switch (t) {
			case MCE_INTR_TYPE_INT:
				printf("Vectored interrupt");
				break;
			case MCE_INTR_TYPE_NMI:
				printf("NMI");
				break;
			case MCE_INTR_TYPE_SMI:
				printf("SMI");
				break;
			case MCE_INTR_TYPE_EXTINT:
				printf("ExtINT");
				break;
			}
			printf(" (%ld)\n", t);
			t = mceioi.mce_io_intr_flags;
			printf("\tFlags             : 0x%lx\n", t);
			printf("\tPolarity          : ");
			switch (t & MCE_INTR_FL_PO) {
			case 0:
				printf("conforms to bus");
				break;
			case 1:
				printf("active high");
				break;
			case 3:
				printf("active low");
				break;
			}
			printf(" (%ld)\n", t & MCE_INTR_FL_PO);
			printf("\tTrigger mode      : ");
			switch ((t & MCE_INTR_FL_EL) >> 2) {
			case 0:
				printf("conforms to bus");
				break;
			case 1:
				printf("edge");
				break;
			case 3:
				printf("level");
				break;
			}
			printf(" (%ld)\n", (t & MCE_INTR_FL_EL) >> 2);
			printf("\tSource bus id     : %d\n",
			    mceioi.mce_io_intr_src_bus_id);
			printf("\tSource bus irq    : %d\n",
			    mceioi.mce_io_intr_src_bus_irq);
			printf("\tDest %s APIC id  : %d\n", at,
			    mceioi.mce_io_intr_dst_apic_id);
			printf("\tDest %s APIC in# : %d\n", at,
			    mceioi.mce_io_intr_dst_apic_int);
			len = MCE_IO_INTR_LEN;
			break;
		default:
			printf("unknown type\n");
			hexdump(mc, m->mct_base_len -((char *)mc - (char *)m));
			len = 0;
			return;
		}
		mc = (union mc *)((char *)mc + len);
	}
	if (m->mct_ext_tab_len == 0)
		return;
	ec = (union ec *)mc;
	if (((mps_cksum(ec, m->mct_ext_tab_len) + m->mct_ext_tab_chks) & 0xff) != 0)
		printf("Extended table checksum failed.  expected %x got %x\n",
		    m->mct_ext_tab_chks, mps_cksum(ec, m->mct_ext_tab_len));
	i = 1;
	while ((char *)ec - (char *)mc < m->mct_ext_tab_len) {
		printf("   Extended Entry %d: ", i++);
		switch (emap.ece_entry_type) {
		case ECE_MAP:
			printf("system address space mapping\n");
			printf("\tBus ID               : %d\n",
			    emap.ece_bus_id);
			printf("\tAddress type         : %d ",
			    emap.ece_addr_type);
			switch (emap.ece_addr_type) {
			case 0:
				printf("I/O");
				break;
			case 1:
				printf("memory address");
				break;
			case 2:
				printf("prefetch address");
				break;
			}
			printf("\n");
			printf("\tBase address         : 0x%8.8lx%8.8lx\n",
				((u_long *)&emap.ece_addr_base)[1],
				((u_long *)&emap.ece_addr_base)[0]);
			printf("\tLength               : 0x%8.8lx%8.8lx\n",
				((u_long *)&emap.ece_addr_len)[1],
				((u_long *)&emap.ece_addr_len)[0]);
			break;
		case ECE_BUS:
			printf("bus heirarchy descriptor\n");
			printf("\tBus ID               : %d\n",
			    ebus.ece_bus_id);
			printf("\tFlags                : 0x%x ",
			    ebus.ece_flags);
			if (ebus.ece_flags & ECE_SUBDEC)
				printf("subtractive decode");
			printf("\n");
			printf("\tParent ID            : %d\n",
			    ebus.ece_parent);
			break;
		case ECE_COMPAT:
			printf("compatibility address space modifier\n");
			printf("\tBus ID               : %d\n",
			    ecompat.ece_bus_id);
			printf("\tFlags                : 0x%x ",
			    ecompat.ece_flags);
			if (ecompat.ece_flags & ECE_PR)
				printf("omit predefined range");
			printf("\n");
			printf("\tPredefined range list: %d ",
				ecompat.ece_pdl);
			switch (ecompat.ece_pdl) {
			case 0:
				printf("ISA");
				break;
			case 1:
				printf("VGA");
				break;
			}
			printf("\n");
			break;
		default:
			printf("unknown type (type %d len %d)\n", 
			    emap.ece_entry_type, emap.ece_entry_len);
			hexdump(ec, emap.ece_entry_len);
		}
		if (emap.ece_entry_len == 0)
			break;
		ec = (union ec *)((char *)ec + emap.ece_entry_len);
	}
}

static void
disp_intr_entry(struct mce_io_intr *ip)
{
	struct mce_bus *busp;
	int irq;

	printf("%4x %3x ", 
	    ip->mce_io_intr_dst_apic_id,
	    ip->mce_io_intr_dst_apic_int);
	switch (ip->mce_io_intr_type) {
	case MCE_INTR_TYPE_INT:
		printf("Vector ");
		break;
	case MCE_INTR_TYPE_NMI:
		printf("NMI\n");
		break;
	case MCE_INTR_TYPE_SMI:
		printf("SMI\n");
		break;
	case MCE_INTR_TYPE_EXTINT:
		printf("ExtINT\n");
		break;
	}
	if (ip->mce_io_intr_type != MCE_INTR_TYPE_INT)
		return;
	switch (ip->mce_io_intr_flags & MCE_INTR_FL_PO) {
	case MCE_PO_CONFORM:
		printf("Conforms ");
		break;
	case MCE_PO_HIGH:
		printf("High     ");
		break;
	case MCE_PO_LOW:
		printf("Low      ");
		break;
	default:
		printf("???      ");
		break;
	}
	switch (ip->mce_io_intr_flags & MCE_INTR_FL_EL) {
	case MCE_FL_CONFORM:
		printf("Conforms ");
		break;
	case MCE_FL_EDGE:
		printf("Edge     ");
		break;
	case MCE_FL_LEVEL:
		printf("Level    ");
		break;
	default:
		printf("???      ");
		break;
	}
	if ((busp = get_bus(ip->mce_io_intr_src_bus_id)) == NULL)
		busp = &defbus;
	printf("%-6.6s (%x) ", busp->mce_bus_type,
	    ip->mce_io_intr_src_bus_id);
	irq = ip->mce_io_intr_src_bus_irq;
	if (strncmp(busp->mce_bus_type, "PCI   ", 6) == 0) {
		printf("Agent 0x%x Pin %c", irq >> 2 & 0x1f, 
		    "ABCD"[irq & 0x3]);
		irq = find_pci_irq(ip, busp);
		if (irq != -1)
			printf(" (maps to irq %x)", irq);
		else
			printf(" (no IRQ)");
	} else if (strncmp(busp->mce_bus_type, "ISA   ", 6) == 0 ||
	    strncmp(busp->mce_bus_type, "EISA  ", 6) == 0) {
		printf("IRQ %x", irq);
	} else {
		printf("??? (%x)", irq);
	}
	printf("\n");
}

/*
 * Display interrupt related info from bootparams
 */
int
disp_intr(int ac, char **av)
{
	int i;
	struct mce_io_intr *ip;
	
	printf("Apic Pin Type   Polarity Mode     Bus        Source\n"
	       "==== === ====== ======== ======== ========== ======\n");
	for (i = 0; (ip = get_intr(i)) != NULL; i++)
		disp_intr_entry(ip);

	return (0);
}

/*
 * Display the MP information in tabular format
 */
int
disp_mpinfo(int ac, char **av)
{
	struct mct *m;
	struct mce_proc *mcep;
	struct mce_bus *mceb;
	struct mce_io_apic *mcea;
	struct mce_lc_intr *lip;
	int i;

	if (fpsp->fps_cfgtype == 0xff)
		printf("No MP configuration, printing default info\n");

	printf("MP Spec revision: 1.%d\n", fpsp->fps_rev);
	switch (fpsp->fps_cfgtype) {
	case 0:
	case 0xff:
		break;
	default:
		printf("Default config: %d\n", fpsp->fps_cfgtype);
		break;
	}
	
	printf("Mode: ");
	if (fpsp->fps_feature[0] & FPS_IMCRP)
		printf(" PIC");
	else
		printf(" Virtual Wire");
	printf("\n");

	if (fpsp->fps_cfg == 0)
		return (0);

	m = (struct mct *)(fpsp + 1);
	printf("OEM ID: %.8s\n", m->mct_oem_id);
	printf("Product ID: %.12s\n",
	    m->mct_prod_id);
	printf("Local APIC addr: 0x%8.8x\n",
	    (u_int)m->mct_local_apic);

	printf("\nProcessors:\n");
	printf("Cpu Vers Type Fam Mdl Step Features Flags\n");
	printf("=== ==== ==== === === ==== ======== =====\n");

	for (i = 0; (mcep = get_ent_bytype(MCE_PROC, i)) != NULL; i++) {
		int family, model;

		family = mcep->mce_proc_cpu_signature >> 8 & 0xf;
		if (family == 0xf)
			family |= mcep->mce_proc_cpu_signature >> 16 & 0xff0;
		model = mcep->mce_proc_cpu_signature >> 4 & 0xf;
		if (model == 0xf)
			model |= mcep->mce_proc_cpu_signature >> 12 & 0xf0;

		printf("%3d %4d %4x %3x %3x %4x %08x",
		    mcep->mce_proc_local_apic_id,
		    mcep->mce_proc_local_apic_vers,
		    mcep->mce_proc_cpu_signature >> 12 & 0x3,
		    family, model,
		    mcep->mce_proc_cpu_signature & 0xf,
		    mcep->mce_proc_cpu_feature);
		if (mcep->mce_proc_cpu_flags & MCE_PROC_FL_EN)
			printf(" enabled");
		if (mcep->mce_proc_cpu_flags & MCE_PROC_FL_BP)
			printf(" bootstrap");
		printf("\n");
	}

	printf("\nSystem buses:\n");
	printf("Bus Type\n");
	printf("=== ======\n");
	for (i = 0; (mceb = get_ent_bytype(MCE_BUS, i)) != NULL; i++) {
		printf("%3d %-6.6s\n",
		    mceb->mce_bus_id,
		    mceb->mce_bus_type);
	}

	printf("\nI/O APICs:\n");
	printf("ID  Vers Address  Flags\n");
	printf("=== ==== ======== =====\n");
	for (i = 0; (mcea = get_ent_bytype(MCE_IO_APIC, i)) != NULL; i++) {
		printf("%3d %4d %08lx",
		    mcea->mce_io_apic_id,
		    mcea->mce_io_apic_vers,
		    (u_long)mcea->mce_io_apic_addr);
		if (mcea->mce_io_apic_flags & MCE_IO_APIC_FL_EN)
			printf(" enabled");
		printf("\n");		
	}

	printf("\nI/O Interrupts:\n");
	disp_intr(0, NULL);

	printf("\nLocal Interrupts:\n");
	printf("Cpu  Pin Type   Polarity Mode     Bus        Source\n"
	       "==== === ====== ======== ======== ========== ======\n");
	for (i = 0; (lip = get_local_intr(i)) != NULL; i++)
		disp_intr_entry((struct mce_io_intr *)lip);

	return (0);
}
