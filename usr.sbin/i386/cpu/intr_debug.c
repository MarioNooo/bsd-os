/*
 * Copyright (c) 1998 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	intr_debug.c,v 1.5 2002/04/04 20:37:33 jch Exp
 */
#include "cpu.h"

static void apic_pinfo __P((char *desc, isrc_apic_t *p, int detail));
static void apic_predir __P((int r));
static void chksrc __P((struct ithdinfo *ithead, isrc_apic_t *chkp));
static void psrcinfo __P((isrc_t *isp, int detail));

/*
 * Debug dump of interrupt route tree
 */
void
piroute(struct ithdinfo *hd)
{
	struct ithdinfo *itp;
	ihand_t *ihp;
	isrc_t *isp;
	char *p;
	struct byidt {
		isrc_t *isp;
		struct ithdinfo *itp;
	} byidt[256];
	int i;

	bzero(byidt, sizeof(byidt));
	for (itp = hd; itp != NULL; itp = itp->it_next) {
		switch (itp->it_prio) {
		case PI_REALTIME:
			p = "RealTime";
			break;
		case PI_AV:
			p = "AV";
			break;
		case PI_TTYHIGH:
			p = "TtyHigh";
			break;
		case PI_TAPE:
			p = "Tape";
			break;
		case PI_NET:
			p = "Network";
			break;
		case PI_DISK:
			p = "Disk";
			break;
		case PI_TTYLOW:
			p = "TtyLow";
			break;
		case PI_DISKLOW:
			p = "DiskLow";
			break;
		case PI_DULL:
			p = "Dull";
			break;
		case PI_SOFT:
			p = "Soft";
			break;
		default:
			p = "";
		}
		printf("Thread: id=%d prio=%s(%d) %s\n", itp->it_id,
		    p, itp->it_prio, itp->it_name);
		LIST_FOREACH(isp, &itp->it_isrchead, is_tlist) {
			printf("\tSRC idt=0x%x flags=0x%x (",
			    isp->is_idt, isp->is_flags);
			if (isp->is_flags & IS_LEVEL)
				printf("level");
			printf(")\n\t\t");
			psrcinfo(isp, 1);
			if (isp->is_idt >= 0x20) {
				if (byidt[isp->is_idt].isp != NULL)
					errx(1, "IDT 0x%x conflict\n",
					    isp->is_idt);
				byidt[isp->is_idt].isp = isp;
				byidt[isp->is_idt].itp = itp;
			}
		}
		LIST_FOREACH(ihp, &itp->it_ihhead, ih_list) {
			switch (ihp->ih_class) {
			case DV_DULL:
				p = "Dull";
				break;
			case DV_CPU:
				p = "Cpu";
				break;
			case DV_DISK:
				p = "Disk";
				break;
			case DV_IFNET:
				p = "Network";
				break;
			case DV_TAPE:
				p = "Tape";
				break;
			case DV_TTY:
				p = "Tty";
				break;
			case DV_CLK:
				p = "Clock";
				break;
			case DV_COPROC:
				p = "Coproc";
				break;
			case DV_SLOWDISK:
				p = "SlowDisk";
				break;
			default:
				p = "InvalidClass";
			}
			printf("\tHND %s %s prio=%d flags=0x%x\n",
			    ihp->ih_xname, p, ihp->ih_prio, ihp->ih_flags);
			printf("\t\tcall 0x%x(0x%x) old 0x%x(0x%x)\n",
			    (u_int)ihp->ih_fun, (u_int)ihp->ih_arg,
			    (u_int)ihp->ih_oldfun, (u_int)ihp->ih_oldarg);
		}
	}
	printf("\nSummary by IDT:\n");
	printf("%4s %-14s %3s %3s  %s\n",
	    "IDT", "Thread", "Pri", "PID", "Source");
	for (i = 0; i < 256; i++) {
		if ((isp = byidt[i].isp) == NULL)
			continue;
		itp = byidt[i].itp;
		printf("0x%2.2x %-14s %3d %3d  ", i, itp->it_name,
		    itp->it_prio, itp->it_id);
		psrcinfo(isp, 0);
	}
}

/*
 * Print line of info about APIC sources
 */
static void
apic_pinfo(char *desc, isrc_apic_t *p, int detail)
{

	printf("%s %u.%u\n", desc, p->is_apic, p->is_pin);
	if (detail == 0)
		return;
	printf("\t\tEna:");
	apic_predir(p->is_enable);
	printf("\t\tDis:");
	apic_predir(p->is_disable);
	printf("\t\tDest = 0x%x\n", p->is_high >> APIC_DF_SHFT);
}

static void
psrcinfo(isrc_t *isp, int detail)
{
	isrc_pic_t *ispp;
	isrc_apic_t *isap;
	isrc_lvt_t *islp;

	isap = (isrc_apic_t *)isp;
	switch (isp->is_type) {
	case PicNormal:
		ispp = (isrc_pic_t *)isp;
		printf("PicNormal Irq=%d pinmask=0x%x\n",
		    ispp->is_irq, ispp->is_pinmask);
		break;
	case ApicNormal:
		apic_pinfo("ApicNormal", isap, detail);
		break;
	case ApicClk:
		apic_pinfo("ApicClk", isap, detail);
		break;
	case ApicClkHack:
		apic_pinfo("ApicClkHack", isap, detail);
		break;
	case ApicUnused:
		apic_pinfo("ApicUnused", isap, detail);
		break;
	case ApicNMI:
		apic_pinfo("ApicNMI", isap, detail);
		break;
	case ApicSMI:
		apic_pinfo("ApicSMI", isap, detail);
		break;
	case LclVecTab:
		islp = (isrc_lvt_t *)isp;
		printf("LclVecTab Cpu=0x%x\n",
		    islp->is_cpuno);
		printf("\t\tLint0: ");
		apic_predir(islp->is_lvt[0]);
		printf("\t\tLint1: ");
		apic_predir(islp->is_lvt[1]);
		break;
	default:
		printf("Unknown Type\n");
		break;
	}
}

/* Decode bits in I/O apic redirection register */
static void
apic_predir(int r)
{
	char *s;

	s = NULL;				/* XXX Shut up -Wall */
	switch (r & APIC_DL_MASK) {
	case APIC_DL_FIXED:
		s = "Fixed";
		break;
	case APIC_DL_LOWPR:
		s = "LowestPrio";
		break;
	case APIC_DL_SMI:
		s = "SMI";
		break;
	case APIC_DL_RR:
		s = "RemoteRead";
		break;
	case APIC_DL_NMI:
		s = "NMI";
		break;
	case APIC_DL_INIT:
		s = "Init";
		break;
	case APIC_DL_START:
		s = "Start";
		break;
	case APIC_DL_EXTINT:
		s = "ExtINT";
		break;
	}
	printf(" vec=0x%x %s", r & APIC_VC_MASK, s);
	if (r & APIC_PL_LOW)
		printf(" Low/");
	else
		printf(" High/");
	if (r & APIC_TM_LEVEL)
		printf("Level");
	else
		printf("Edge");
	if (r & APIC_RIRR)
		printf(" RIRR");
	if (r & APIC_DS_BUSY)
		printf(" BUSY");
	if (r & APIC_DM_LOGI)
		printf(" logical");
	else
		printf(" physical");
	if (r & APIC_MS_DISABLE)
		printf(" Disabled");
	switch (r & APIC_DH_MASK) {
	case APIC_DH_DSTF:
		printf(" -> Dest");
		break;
	case APIC_DH_SELF:
		printf(" -> Self");
		break;
	case APIC_DH_ALLS:
		printf(" -> All");
		break;
	case APIC_DH_ALLE:
		printf(" -> AllButSelf");
		break;
	}
	printf("\n");
}

/*
 * Perform paranoid sanity checks on tree going back to kernel
 */
void
intr_sanity(void)
{
	struct ithdinfo *itp;

	if (vflag)
		printf("\nPerforming sanity checks:\n");

	/* Make sure each thread has at least one source */
	for (itp = nit; itp != NULL; itp = itp->it_next)
		if (LIST_EMPTY(&itp->it_isrchead))
			errx(1, "No intr sources on thread %s "
			    "(internal error)\n", itp->it_name);

	/* Make sure there aren't more than 1 source for a given apic/pin */
	chksrc(nit, NULL);
}

/*
 * Loop through all APIC sources, make sure there are no dup pins
 */
static void
chksrc(struct ithdinfo *ithead, isrc_apic_t *chkp)
{
	struct ithdinfo *itp;
	isrc_t *isp;
	isrc_apic_t *ispp;

	for (itp = ithead; itp != NULL; itp = itp->it_next) {
		LIST_FOREACH(isp, &itp->it_isrchead, is_tlist) {
			if (isp->is_type == LclVecTab)
				continue;
			if (isp->is_type == PicNormal)
				errx(1, "pic source in sio intr list "
				    "(internal err)");
			ispp = (isrc_apic_t *)isp;
			if (chkp != NULL) {
				if (ispp == chkp)
					return;
				if (ispp->is_apic == chkp->is_apic &&
				    ispp->is_pin == chkp->is_pin &&
				    vflag > 1)
					warnx("multiple sources on "
					    "apic=%d pin=%d", ispp->is_apic,
					    ispp->is_pin);
			} else
				chksrc(nit, ispp);
		}
	}
}
