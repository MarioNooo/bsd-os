/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI idiff.c,v 1.2 2003/03/10 21:14:32 dab Exp
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <kvm.h>
#include <nlist.h>
#include <stdlib.h>
#include <limits.h>
#include <a.out.h>

#define KERNEL
#include <sys/file.h>
#undef KERNEL


#define USAGE "Usage: %s -N namelist -M memfile\n"

struct nlist    nl[] = {
	{"SYSTEM"},		/* 0 */
	{"_etext"},		/* 1 */
	{"bootparamp"},		/* 2 */
	{"cpu_id"},		/* 3 */
	{"cpu_vendor"},		/* 4 */
	{"cpu_feature_flags"},	/* 5 */
	{"cpu"},		/* 6 */
	{NULL},
};
#define	FIRST_EX	2
#define	LAST_EX		6
#define	CPU_VENDOR	4

kvm_t          *kvmd;

int main __P((int ac, char **av));
void usage __P((char *self));
int check_ex __P((u_int addr));

#define NSYMS	(sizeof(nl)/sizeof(struct nlist))-1

int
main(int ac, char **av)
{
	int ch;
	char buf[1024];
	char *nlistf = NULL;
	char *memf = NULL;
	struct device dv;
	int i;
	struct exec e;
	int tsize;
	int kfd;
	u_char *kp;
	u_char *mp;
	u_char *m, *k;
	int hdr = 0;
	off_t offs;
	u_int kva;

	while ((ch = getopt(ac, av, "N:M:d:")) != EOF) {
		switch (ch) {
		case 'N':
			nlistf = optarg;
			break;
		case 'M':
			memf = optarg;
			break;
		default:
			usage(av[0]);
		}
	}
	if (nlistf == NULL)
		nlistf = "/bsd";
	ac -= optind;
	if (ac != 0)
		usage(av[0]);

	/* Attach to kernel in question */
	if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == NULL) {
		fprintf(stderr, "kvm_open: %s\n", buf);
		exit(1);
	}
	/* Get the kernel symbols */
	if (kvm_nlist(kvmd, nl) != 0) {
		for (i = 0; i < NSYMS; i++)
			if (nl[i].n_type == 0)
				fprintf(stderr, "kvm_nlist: %s not found\n",
					nl[i].n_name);
		exit(1);
	}
	printf("SYSTEM=%x\n", nl[0].n_value);
	printf("_etext=%x\n", nl[1].n_value);
	tsize = nl[1].n_value - nl[0].n_value;

	/* Open kernel with known good text */
	if ((kfd = open(nlistf, O_RDONLY)) < 0)
		err(1, "open %s failed", nlistf);
	/* Get offset of start of kernel in file */
	offs = _nlist_offset(kfd, nl[0].n_value);
	if (lseek(kfd, offs, SEEK_SET)  == -1)
		err(1, "lseek on kernel image");
	printf("Text size = %d\n", tsize);

	/* Quick and hack could be used to describe this... */
	kp = malloc(tsize);
	printf("Reading text from %s\n", nlistf);
	if (read(kfd, kp, tsize) != tsize)
		err(1, "kernel file text read");

	mp = malloc(tsize);
	printf("Reading text from memory image\n");
	if (kvm_read(kvmd, nl[0].n_value, mp, tsize) != tsize)
		err(1, "image text read");

	printf("Comparing...\n");
	m = mp;
	k = kp;
	while (m != &mp[tsize]) {
		if (*m != *k) {
			kva = m - mp + nl[0].n_value;
			if (check_ex(kva) == 0) {
				if (!hdr) {
					hdr = 1;
					printf("KVA         Exp Act Diff\n");
				}
				printf("0x%8.8x: %2.2x  %2.2x  %2.2x\n",
				    kva, *k, *m, *k ^ *m);
			}
		}
		m++;
		k++;
	}
	
}

void
usage(char *self)
{
	fprintf(stderr, USAGE, self);
	exit(1);
}

int
check_ex(u_int addr)
{
	int i;
	int len;

	for (i = FIRST_EX; i <= LAST_EX; i++) {
		len = 4;
		if (i == CPU_VENDOR)
			len = 12;
		if (addr >= nl[i].n_value &&
		    addr < nl[i].n_value + len)
			return (1);
	}
	return (0);
}

