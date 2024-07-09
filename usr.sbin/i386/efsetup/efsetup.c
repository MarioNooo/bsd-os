/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI efsetup.c,v 1.2 1996/05/09 00:07:18 ewv Exp
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define EF_TYPE_ARRAY
#include <i386/isa/if_efreg.h>
#include <i386/isa/efioctl.h>

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

#define USAGE "Usage: %s [options] ifname\n\
	-m type		Select media type at powerup\n\
				tp	- 10baseT\n\
				aui	- AUI\n\
				bnc	- 10base2\n\
				tx	- 100baseTX\n\
				fx	- 100baseFX\n\
				mii	- Media independent interface\n\
				t4	- 100baseT4 (via MII)\n\
	-p ratio	Buffer ratio (read:write) (5:3, 3:1, 1:1)\n\
	-s		Enable 100 mbit SSD corruption checking\n\
	-S		Disable SSD corruption checking\n\
	-v		Dump detailed EEPROM info (do not make any changes)\n"

int main(int ac, char **av);
void dumpinfo(void);
struct eftype *findid(int id);
u_short rprom(int addr);
void wprom(int addr, u_short data);
u_short getcsum(void);
void usage(char *name);

int fd;
struct ifreq ifr;
struct msel {
	char *key;
	char *name;
	int val;
	int linkflags;
	int resopt;
} msel[] = {
	{ "tp",		"10BaseT",			0, 2, EFRO_TP },
	{ "aui",	"10Base5 (AUI)",		1, 0, EFRO_AUI },
	{ "bnc",	"10Base2 (BNC)",		3, 1, EFRO_BNC },
	{ "tx",		"100BaseTX",			4, 4, EFRO_TX },
	{ "fx",		"100BaseFX",			5, 5, EFRO_FX },
	{ "mii",	"MII",				6, 6, EFRO_MII },
	{ "t4",		"100BaseT4",			6, 6, EFRO_T4 },
	{ NULL, 	"Unknown" }
};

/* Valid combinations of ram size, ram width, and ram partitioning */
#define	V(s,w,p) \
	s << EFIC_RAMSIZE | w << EFIC_RAMWIDTH | p << EFIC_RAMPART
int valpart[] = {
	V(EFIC_8K,	EFIC_BYTE,	EFIC_11),
	V(EFIC_8K,	EFIC_BYTE,	EFIC_31),
	V(EFIC_8K,	EFIC_BYTE,	EFIC_53),
	V(EFIC_32K,	EFIC_BYTE,	EFIC_11),
	V(EFIC_64K,	EFIC_WORD,	EFIC_11),
	V(EFIC_64K,	EFIC_WORD,	EFIC_31),
	V(EFIC_128K,	EFIC_WORD,	EFIC_11)
};

#define	VALPART_SIZE	sizeof(valpart) / sizeof(int)

int
main(int ac, char **av)
{
	int c;
	char *const name = av[0];
	int vflag = 0;
	int pinfo = 0;
	int setssd = 0;
	int ssdstate;
	char *setm = NULL;
	char *setpart = NULL;
	struct eftype *ep;
	int i;
	struct msel *mp;
	int ic;
	int ic_orig;

	while ((c = getopt(ac, av, "m:p:sSv")) != EOF) {
		switch (c) {
		case 'm':
			setm = optarg;
			break;
		case 'p':
			setpart = optarg;
			break;
		case 's':
			setssd = 1;
			ssdstate = 0;
			break;
		case 'S':
			setssd = 1;
			ssdstate = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			usage(name);
		}
	}

	if (ac == 2)
		pinfo = 1;
		
	ac -= optind;

	if (ac != 1)
		usage(name);

	strncpy(ifr.ifr_name, av[optind], IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = 0;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "can't create socket");

	/*
	 * Dump the world if verbose
	 */
	if (vflag) {
		dumpinfo();
		exit(0);
	}

	/*
	 * Figure out what kind of card we're dealing with here
	 */
	i = rprom(EFROM_PID);
	ep = findid(i);
	if ((ep->ef_flags & EF_NWEINT) == 0) {
		fprintf(stderr, "Card type (%s) not supported by %s\n",
			ep->ef_name, av[0]);
		exit(1);
	}

	/*
	 * Change requested options
	 */
	ic = ic_orig = rprom(EFROM_ICL) | rprom(EFROM_ICH) << 16;
	if (setm != NULL) {
		for (mp = msel; mp->key != NULL; mp++)
			if (!strcmp(mp->key, setm))
				break;
		if (mp == NULL) {
			fprintf(stderr, "%s: %s is not a valid media type\n",
			    av[0], setm);
			exit(1);
		}
		if ((getresopt() & mp->resopt) == 0) {
			fprintf(stderr, "%s: %s does not support %s\n",
			    av[0], av[optind], mp->name);
			exit(1);
		}
		ic = ic & ~(0x7 << EFIC_MSEL) | mp->val << EFIC_MSEL;
	}
	if (setpart != NULL) {
		if (strcmp(setpart, "5:3") == 0)
			i = EFIC_53;
		else if (strcmp(setpart, "3:1") == 0)
			i = EFIC_31;
		else if (strcmp(setpart, "1:1") == 0)
			i = EFIC_11;
		else {
			fprintf(stderr, "%s: Invalid RAM partition ratio\n",
			    av[0]);
			exit(1);
		}
		ic = ic & ~(0x3 << EFIC_RAMPART) | i << EFIC_RAMPART;
		/* Make sure this is a valid partitioning */
		c = ic & (0x3 << EFIC_RAMPART | 1 << EFIC_RAMWIDTH |
		    0x7 << EFIC_RAMSIZE);
		for (i = 0; i < VALPART_SIZE; i++)
			if (c == valpart[i])
				break;
		if (i == VALPART_SIZE) {
			fprintf(stderr, "%s: Invalid combination of ram size, "
			    "width, and partition ratio\n", av[0]);
			exit(1);
		}
	}
	if (setssd == 1) {
		ic &= ~(1 << EFIC_BADSSD);
		ic |= ssdstate << EFIC_BADSSD;
	}
	if (ic == ic_orig) {
		printf("No changes made to %s\n", av[optind]);
	} else {
		if ((ic & 1 << EFIC_AUTOSEL) != 0) {
			printf("Warning: broken autoselect feature enabled, "
			    "turning it off\n");
			ic &= ~(1 << EFIC_AUTOSEL);
		}
		if ((ic & 0xffff) != (ic_orig & 0xffff))
			wprom(EFROM_ICL, ic & 0xffff);
		if (ic >> 16 != ic_orig >> 16)
			wprom(EFROM_ICH, ic >> 16);
		wprom(EFROM_CSUM, getcsum());
		printf("EEPROM programmed.\n");
	}
	return (0);
}

/*
 * Dump detailed info
 */
void
dumpinfo()
{
	int i;
	struct msel *mp;
	int ram;
	int rram;
	char *p;
	u_short d;
	struct eftype *ep;
	int ropt;

	i = rprom(EFROM_PID);
	ep = findid(i);
	printf("Product ID            : %s (0x%4.4x)\n", ep->ef_name, i);
	ropt = getresopt();
	printf("Media supported       : ");
	p = "";
	for (mp = msel; mp->key != NULL; mp++) {
		if ((ropt & mp->resopt) != 0)
			printf("%s%s", p, mp->name);
		p = ", ";
	}
	printf("\n");
	i = rprom(EFROM_ICL) | rprom(EFROM_ICH) << 16;
	for (mp = msel; mp->key != NULL ; mp++) {
		if (mp->val == (i >> EFIC_MSEL & 0x7))
			break;
	}
	printf("Media select on reset : %s\n", mp->name);
	ram = 8 << (i & 0x7);
	printf("Ram size              : %d Kb\n", ram);
	printf("Ram width             : %s\n",
	    (i & (1 << EFIC_RAMWIDTH)) ? "word" : "byte");
	printf("Ram wait states       : %d\n", i >> EFIC_RAMSPEED & 0x3);
	printf("Rom size              : %d Kb\n",
	    8 << (i >> EFIC_ROMSIZE & 0x3));
	printf("BadSSD detect         : %s\n",
	    i & 1 << EFIC_BADSSD ? "off" : "on");
	switch (i >> EFIC_RAMPART & 0x3) {
	case 0:
		p = "5:3";
		rram = (ram >> 3) * 5;
		break;
	case 1:
		p = "3:1";
		rram = (ram >> 2) * 3;
		break;
	case 2:
		p = "1:1";
		rram = ram >> 1;
		break;
	case 3:
		p = "reserved";
		rram = ram >> 1;
		break;
	}
	printf("Buffer ratio          : %s\n", p);
	printf("Read FIFO size        : %d Kb\n", rram);
	printf("Write FIFO size       : %d Kb\n", ram - rram);
	printf("Media autoselect      : %s\n",
	    (i & 1 << EFIC_AUTOSEL) ? "on" : "off");
	printf("\nEEPROM dump:");
	for (i = 0; i < 64; i++) {
		if ((i & 7) == 0)
			printf("\n\t%2.2x:", i);
		d = rprom(i);
		printf(" %4.4x", d);
	}
	printf("\n");
	if (rprom(EFROM_CSUM) != getcsum())
		printf("*** Checksum mismatch ***\n");
}

/*
 * Find PID in table
 */
struct eftype *
findid(int i)
{
	struct eftype *ep;

	for (ep = ef_types; ep->ef_deviceid != 0; ep++)
		if ((i & ep->ef_devmask) == ep->ef_deviceid)
			break;
	return(ep);
}

/*
 * Read from the prom
 */
u_short
rprom(int addr)
{
	struct efiocargs *ep = (struct efiocargs *)&ifr.ifr_ifru;

	ep->addr = addr;
	if (ioctl(fd, EFIOCRPROM, (caddr_t)&ifr) < 0)
		err(1,"prom read failed: addr=0x%x", addr);
	return (ep->data);
}

/*
 * Write to prom
 */
void
wprom(int addr, u_short data)
{
	struct efiocargs *ep = (struct efiocargs *)&ifr.ifr_ifru;

	ep->addr = addr;
	ep->data = data;
	if (ioctl(fd, EFIOCWPROM, (caddr_t)&ifr) < 0)
		err(1,"prom write failed: addr=0x%x data=0x%x", addr, data);
	return;
}

/*
 * Compute EEPROM checksum
 */
u_short
getcsum()
{
	int i;
	int a;
	int v;

	a = 0;
	for (i = 0; i < 0x17; i++) {
		v = rprom(i);
		a ^= v & 0xff;
		a ^= v >> 8;
	}
	return (a);
}

/*
 * Retrieve reset options
 */
int
getresopt()
{
	if (ioctl(fd, EFIOCRROPT, (caddr_t)&ifr) < 0)
		err(1,"get reset options failed");
	return (ifr.ifr_ifru.ifru_metric);
}

/*
 * Print usage and exit
 */
void
usage(char *name)
{
	fprintf(stderr, USAGE, name);
	exit(1);
}
