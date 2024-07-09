/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	kerninfo.c,v 1.9 2002/04/04 21:38:33 jch Exp
 */
#include "cpu.h"

FILE *ifp;
struct bpdata *bphead;
struct pcidata *phead;
struct ithdinfo *ihead;
struct miscinfo miscinfo;

#define	VERSION 0x05350536

static void wchunk __P((void *, int, char *));
static void rchunk __P((void *, int, char *));


/*
 * Gather information from the kernel
 */
void
get_kern_info()
{
	int i;
	int s[5];
	int thd;
	int sz;
	int res[2];

	/*
	 * Get all bootparams
	 */
	s[0] = CTL_KERN;
	s[1] = KERN_BOOTPARAMS;
	s[2] = B_INTEL_MP;
	s[3] = 0;
	sz = 0;
	if (sysctl(s, 4, NULL, &sz, NULL, 0) < 0) {
		if (errno != EOPNOTSUPP)
			err(1, "KERN_BOOTPARAMS sysctl failed");
		warnx("No MP data available, using default config");
		sz = 0;
	}
	if (sz != 0) {
		fpsp = malloc(sz);
		if (sysctl(s, 4, fpsp, &sz, NULL, 0) < 0)
			err(1, "KERN_BOOTPARAMS sysctl failed");
		fpsp_len = sz;
	}

	/*
	 * Download current interupt routing tree
	 */
	s[0] = CTL_MACHDEP;
	s[1] = CPU_INTR;
	for (thd = 0; ; thd++) {
		struct ithdinfo ii;
		struct ithdinfo *ipi;

		s[2] = CPUINTR_THREAD;
		s[3] = thd;
		sz = sizeof(ii);
		if (sysctl(s, 4, &ii, &sz, NULL, 0) < 0) {
			if (errno == ENOENT)
				break;
			err(1, "CPUINTR_THREAD sysctl");
		}
		ipi = malloc(sizeof(ii));
		*ipi = ii;
		ipi->it_next = ihead;
		ihead = ipi;
		LIST_INIT(&ipi->it_ihhead);
		LIST_INIT(&ipi->it_isrchead);

		/* Don't look for sources for soft interrupt threads */
		if (ipi->it_flag & P_SITHD)
			continue;

		/* Get sources */
		for (i = 0; ; i++) {
			char buf[ISMAXSIZE];
			isrc_t *isp = (isrc_t *)buf;
			isrc_t *isrcp;

			s[2] = CPUINTR_SRC;
			s[3] = ipi->it_id;
			s[4] = i;
			sz = ISMAXSIZE;
			if (sysctl(s, 5, buf, &sz, NULL, 0) < 0) {
				if (errno == ENOENT)
					break;
				err(1, "CPUINTR_SRC sysctl");
			}
			isrcp = malloc(sz);
			bcopy(isp, isrcp, sz);
			LIST_INSERT_HEAD(&ipi->it_isrchead, isrcp, is_tlist);
		}
		if (i == 0)
			errx(1, "No interrupt sources on thd pid=%d %s\n",
			    ipi->it_id, ipi->it_name);

		/* Get handlers */
		for (i = 0; ; i++) {
			ihand_t ih;
			ihand_t *ihp;

			s[2] = CPUINTR_HAND;
			s[3] = ipi->it_id;
			s[4] = i;
			sz = sizeof(ih);
			if (sysctl(s, 5, &ih, &sz, NULL, 0) < 0) {
				if (errno == ENOENT)
					break;
				err(1, "CPUINTR_HAND sysctl");
			}
			ihp = malloc(sizeof(ih));
			bcopy(&ih, ihp, sizeof(ih));
			LIST_INSERT_HEAD(&ipi->it_ihhead, ihp, ih_list);
		}
		if (i == 0)
			errx(1, "No interrupt handlers on thd pid=%d %s\n",
			    ipi->it_id, ipi->it_name);

	}
	if (ihead == NULL)
		errx(1, "Kernel doesn't report any interrupt sources");

	/*
	 * PCI info
	 */
	s[0] = CTL_MACHDEP;
	s[1] = CPU_PCI;
	for (i = 0; ; i++) {
		struct cpu_pciinfo p;
		struct pcidata *pp;
		int sz = sizeof(p);

		s[2] = i;
		if (sysctl(s, 3, &p, &sz, NULL, 0) < 0)
			break;
		pp = malloc(sizeof(*pp));
		pp->pi = p;
		pp->next = phead;
		phead = pp;
	}

	/* Misc info */
	s[1] = CPU_SIOMODE;
	sz = sizeof(res);
	if (sysctl(s, 2, res, &sz, NULL, 0) < 0 &&
	    errno != ENOMEM)
		err(1, "fetch CPU_SIOMODE");
	miscinfo.siomode = res[0];
	s[1] = CPU_NCPU;
	if (sysctl(s, 2, &miscinfo.ncpu, &sz, NULL, 0) < 0)
		err(1, "fetch CPU_NCPU");
	for (i = 0; i < MAXCPUS; i++) {
		s[1] = CPU_CPU;
		s[2] = i;
		miscinfo.cpustate[i] = CPU_UNKNOWN;
		if (sysctl(s, 3, &miscinfo.cpustate[i], &sz, NULL, 0) < 0 &&
		    errno != ENOENT)
			err(1, "fetch CPU_CPU for %d", i);
	}
	s[1] = CPU_KTRMASK;
	if (sysctl(s, 2, &miscinfo.ktrmask, &sz, NULL, 0) < 0)
		miscinfo.ktrmask = 0;
	s[1] = CPU_BOOTCPU;
	if (sysctl(s, 2, &miscinfo.bootcpu, &sz, NULL, 0) < 0)
		err(1, "fetch CPU_BOOTCPU");

	/* Extract MP table info */
	get_mptable();
}

/*
 * Save kernel state to a file
 */
int
save_info(int ac, char **av)
{
	char cbuf[128];
	ihand_t *ihp;
	int i;
	int sz;
	isrc_t *isp;
	struct ithdinfo *itp;
	struct pcidata *pcp;

	if (ac != 2)
		errx(1, "Usage: save filename");
	snprintf(cbuf, sizeof(cbuf), "uuencode -m cpudata > %s", av[1]);
	if ((ifp = popen(cbuf, "w")) == NULL)
		err(1, "Can't encode %s", av[1]);

	/* Version */
	i = VERSION;
	wchunk(&i, sizeof(i), "version");

	/*
	 * Boot param data
	 */
	wchunk(&fpsp_len, sizeof(fpsp_len), "mpinfo len");
	wchunk(fpsp, fpsp_len, "mpinfo data");

	/*
	 * Interrupt routing tree
	 */
	if (ihead == NULL)
		errx(1, "save: no intr entries: internal error");
	for (itp = ihead; itp != NULL; itp = itp->it_next) {
		wchunk(itp, sizeof(*itp), "ithd info");
		LIST_FOREACH(isp, &itp->it_isrchead, is_tlist) {
			sz = ISSIZE(isp->is_type);
			wchunk(&sz, sizeof(sz), "isrc size");
			wchunk(isp, sz, "isrc entry");
		}
		LIST_FOREACH(ihp, &itp->it_ihhead, ih_list)
			wchunk(ihp, sizeof(*ihp), "ihand entry");
	}

	/* 
	 * PCI info
	 */
	for (pcp = phead; pcp != NULL; pcp = pcp->next)
		wchunk(pcp, sizeof(*pcp), "pci info");

	/* Cpu status */
	wchunk(&miscinfo, sizeof(miscinfo), "miscinfo");

	pclose(ifp);

	return (0);
}

/*
 * Load kernel state from a file
 */
void
load_info(char *filename)
{
	char cbuf[128];
	ihand_t *ihp;
	ihand_t *lihp;
	int i;
	int sz;
	isrc_t *isp;
	isrc_t *lisp;
	struct ithdinfo *itp;
	struct ithdinfo *litp;
	struct ithdinfo *nitp;
	struct pcidata *pcp;
	void *tptr;

	if (access(filename, O_RDONLY) <0)
		err(1, "Can't read %s", filename);
	snprintf(cbuf, sizeof(cbuf), "uudecode -o /dev/stdout < %s", filename);
	if ((ifp = popen(cbuf, "r")) == NULL)
		err(1, "Can't decode %s", filename);

	/* Check version */
	rchunk(&i, sizeof(i), "version");
	if (i != VERSION)
		errx(1, "save file version mismatch (exp=%d act=%d)\n",
		    VERSION, i);

	/*
	 * Boot param data
	 */
	if (fpsp != NULL)
		errx(1, "load_info: fpsp != NULL (internal error)");
	rchunk(&i, sizeof(i), "mpinfo len");
	if (i < 0 || i > BOOT_MAXPARAMS)
		errx(1, "invalid bootparam data len (%d)", i);
	fpsp = malloc(i);
	rchunk(fpsp, i, "mpinfo data");
	if (strncmp(fpsp->fps_c_sign, FPS_CHAR_SIGN, 4))
		errx(1, "Bad FPS signature: %4.4s", fpsp->fps_c_sign);
	fpsp_len = i;

	/*
	 * Interrupt routing tree
	 */
	litp = NULL;
	do {
		itp = malloc(sizeof(*itp));
		rchunk(itp, sizeof(*itp), "ithd entry");
		nitp = itp->it_next;
		itp->it_next = NULL;
		if (litp == NULL)
			ihead = itp;
		else
			litp->it_next = itp;
		litp = itp;
		/* Sources */
		if (!LIST_EMPTY(&itp->it_isrchead)) {
			LIST_INIT(&itp->it_isrchead);
			lisp = NULL;
			do {
				rchunk(&sz, sizeof(sz), "isrc size");
				isp = malloc(sz);
				rchunk(isp, sz, "isrc entry");
				tptr = LIST_NEXT(isp, is_tlist);
				if (lisp == NULL)
					LIST_INSERT_HEAD(&itp->it_isrchead, isp,
					    is_tlist);
				else
					LIST_INSERT_AFTER(lisp, isp, is_tlist);
				lisp = isp;
			} while (tptr != NULL);
		}
		/* Handlers */
		if (!LIST_EMPTY(&itp->it_ihhead)) {
			LIST_INIT(&itp->it_ihhead);
			lihp = NULL;
			do {
				ihp = malloc(sizeof(*ihp));
				rchunk(ihp, sizeof(*ihp), "ihand entry");
				tptr = LIST_NEXT(ihp, ih_list);
				if (lihp == NULL)
					LIST_INSERT_HEAD(&itp->it_ihhead, ihp,
					    ih_list);
				else
					LIST_INSERT_AFTER(lihp, ihp, ih_list);
				lihp = ihp;
			} while (tptr != NULL);
		}
	} while (nitp != NULL);

	/*
	 * PCI info
	 */
	do {
		pcp = malloc(sizeof(*pcp));
		rchunk(pcp, sizeof(*pcp), "pci info entry");
		tptr = pcp->next;
		pcp->next = phead;
		phead = pcp;
	} while (tptr != NULL);

	/* Cpu status */
	rchunk(&miscinfo, sizeof(miscinfo), "miscinfo");

	pclose(ifp);

	/* Find MP table info */
	get_mptable();
}

void
wchunk(void *data, int len, char *desc)
{

	if (vflag > 2)
		printf("wchunk: %s: %u bytes\n", desc, len);

	if (fwrite(data, len, 1, ifp) != 1)
		errx(1, "Error writing chunk %s", desc);
}

void
rchunk(void *data, int len, char *desc)
{
	if (vflag > 2)
		printf("rchunk: %s: %u bytes\n", desc, len);

	if (fread(data, len , 1, ifp) != 1)
		errx(1, "%s reading chunk %s", 
		    feof(ifp) ? "EOF" : "Error", desc);
}
