/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI tesetup.c,v 1.3 1996/06/25 19:33:08 ewv Exp
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <i386/isa/teioctl.h>

#include <time.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EMSG sys_errlist[errno]

#define USAGE "Usage: %s ifname\n\
       %s [options]\n\
	-i file		Input microcode binary file (ex: smc825uc.bin)\n\
	-I file		Input %s executable (def: /usr/sbin/tesetup)\n\
	-o file		Output (updated) %s executable (def: ntesetup)\n"

char	*ifile = NULL;
char	*ofile = "ntesetup";
char	*iex = "/usr/sbin/tesetup";
extern	u_short smc_ucode[];

caddr_t findeye(caddr_t const eye, caddr_t addr, int len);
int update(void);
void usage(char *const name);

int
main(int ac, char **av)
{
	int c;
	char *const name = av[0];
	struct ifreq ifr;
	int fd;

	while ((c = getopt(ac, av, "i:I:o:")) != EOF) {
		switch (c) {
		case 'i':
			ifile = optarg;
			break;
		case 'I':
			iex = optarg;
			break;
		case 'o':
			ofile = optarg;
			break;
		default:
			usage(name);
		}
	}

	av += optind;
	ac -= optind;

	if (ifile != NULL && ac != 0 ||
	    ifile == NULL && ac != 1)
		usage(name);

	if (ifile)
		return (update());
	
	strncpy(ifr.ifr_name, av[0], IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = 0;
	ifr.ifr_data = (caddr_t)&smc_ucode[3];

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr,"Can't create socket: %s\n", EMSG);
		return (1);
	}
	if (ioctl(fd, TEIOCUCODE, (caddr_t)&ifr)) {
		fprintf(stderr,"Microcode load failed: %s\n", EMSG);
		return (1);
	}
	return (0);
}

/*
 * Update our own binary with new microcode from floppy
 */
int
update()
{
	int fd;
	caddr_t iaddr;
	caddr_t iuc;
	u_short	*tmp;
	u_short sum;
	caddr_t oaddr;
	caddr_t ouc;
	struct stat st;
	int i;
	const int blen = strlen(ofile) + strlen(iex) + 5;
	char *cmd;

	cmd = malloc(blen);
	if (!cmd) {
		fprintf(stderr,"Malloc failed\n");
		return (1);
	}
	sprintf(cmd,"cp %s %s",iex,ofile);
	if (system(cmd)) {
		fprintf(stderr,"Command \"%s\" failed\n",cmd);
		return (1);
	}

	/*
	 * Map in the source file
	 */
	if ((fd = open(ifile, O_RDONLY)) < 0) {
		fprintf(stderr,"Error opening %s: %s\n", ifile, EMSG);
		return (1);
	}
	if (fstat(fd, &st) < 0) {
		fprintf(stderr,"Error stating %s: %s\n", ifile, EMSG);
		return (1);
	}
	if ((iaddr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0)) 
	    == NULL) {
		fprintf(stderr,"Error mapping %s: %s\n", ifile, EMSG);
		return (1);
	}

	/*
	 * Find the microcode and validate it
	 */
	if ((iuc = findeye((caddr_t)"microcode here>", 
				iaddr, st.st_size)) == NULL) {
		fprintf(stderr,"Eyecatcher not found in %s\n", ifile);
		return (1);
	}
	iuc += 16;		/* Start of ucode */
	if ((iuc + SM_UCODE_SIZE) - iaddr > st.st_size) {
		fprintf(stderr,"File %s has short microcode section\n", ifile);
		return (1);
	}
	tmp = (u_short *)iuc;
	sum = 0;
	for (i = 0; i < SM_UCODE_SIZE / sizeof(u_short); i++)
		sum += tmp[i];
	if (sum != 0) {
		fprintf(stderr,"Invalid microcode checksum in %s\n", ifile);
		return (1);
	}

	/*
	 * Map in the target executable and find start of ucode area
	 */
	if ((fd = open(ofile, O_RDWR, 0750)) < 0) {
		fprintf(stderr,"Error opening %s: %s\n", ofile, EMSG);
		return (1);
	}
	if (fstat(fd, &st) < 0) {
		fprintf(stderr,"Error stating %s: %s\n", ofile, EMSG);
		return (1);
	}
	if ((oaddr = mmap(0, st.st_size, PROT_WRITE|PROT_READ, 
					MAP_SHARED, fd, 0)) == NULL) {
		fprintf(stderr,"Error mapping %s: %s\n", ofile, EMSG);
		return (1);
	}
	close(fd);
	if ((ouc = findeye((caddr_t)smc_ucode, oaddr, st.st_size)) == NULL) {
		fprintf(stderr,"Eyecatcher not found in %s\n", ofile);
		return (1);
	}
	ouc += 6;

	/*
	 * Copy the new microcode in
	 */
	bcopy(iuc, ouc, SM_UCODE_SIZE);
	return (0);
}

/*
 * Find an eyecatcher in a region of memory
 */
caddr_t
findeye(caddr_t const eye, caddr_t addr, int len)
{
	const int eyelen = strlen(eye);

	while (len >= eyelen) {
		if ((*eye == *addr) && (bcmp(eye,addr,eyelen) == 0))
			return (addr);
		addr++;
		len--;
	}
	return (0);
}

/*
 * Print usage and exit
 */
void
usage(char *const name)
{
	fprintf(stderr, USAGE, name, name, name, name);
	exit(1);
}
