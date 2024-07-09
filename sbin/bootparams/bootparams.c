/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 2001 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI bootparams.c,v 1.3 2002/01/29 18:15:19 donn Exp
 */
#include <sys/param.h>
#include <sys/device.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/reboot.h>
#ifdef	B_ACPI_SMAP
#include <machine/acpi.h>
#endif
#ifdef	B_BIOSPM
#include <machine/pmparam.h>
#endif
#ifdef	B_ISAPARAM
#include <i386/isa/isa.h>
#include <i386/isa/isavar.h>
#endif

#include <net/if.h>
#include <net/if_config.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <nfs/nfsproto.h>
#include <nfs/nfsdiskless.h>

#include <err.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <vis.h>

#ifdef	B_ACPI_SMAP
static void printacpismap(char *, char *, void *, int);
#endif
#ifdef	B_AUTODEBUG
static void printautodebug(char *, char *, void *, int);
#endif
#ifdef	B_BIOSGEOM
static void printbiosgeom(char *, char *, void *, int);
#endif
#ifdef	B_BIOSINFO
static void printbiosinfo(char *, char *, void *, int);
#endif
#ifdef	B_BIOSPM
static void printbiospm(char *, char *, void *, int);
#endif
#ifdef	B_BOOTPVENDOR
static void printbootp(char *, char *, void *, int);
#endif
#ifdef	B_BSDGEOM
static void printbsdgeom(char *, char *, void *, int);
#endif
#ifdef	B_CONSOLE
static void printconsole(char *, char *, void *, int);
#endif
#ifdef	B_DEVSPEC
static void printdevspec(char *, char *, void *, int);
#endif
#ifdef	B_HOWTO
static void printhowto(char *, char *, void *, int);
#endif
#ifdef	B_IFCONFIG
static void printifconfig(char *, char *, void *, int);
#endif
#ifdef	B_ISAPARAM
static void printisaparam(char *, char *, void *, int);
#endif
#ifdef	B_KDEBUG
static void printkdebug(char *, char *, void *, int);
#endif
#ifdef	B_KERNSPACE
static void printkernspace(char *, char *, void *, int);
#endif
#if	defined(B_MEM) || defined(B_MEMFS)
static void printmem(char *, char *, void *, int);
#endif
#ifdef	B_NFSPARAMS
static void printnfs(char *, char *, void *, int);
#endif
#ifdef	B_PARAMFOR
static void printparamfor(char *, char *, void *, int);
#endif
#ifdef	B_PNPID
static void printpnpid(char *, char *, void *, int);
#endif
#ifdef	B_REALBOOTDEV
static void printrealbootdev(char *, char *, void *, int);
#endif
#ifdef	B_ROOTTYPE
static void printroottype(char *, char *, void *, int);
#endif

static void dump_mem(unsigned char *, int);
static void *getdata(int, int, size_t *);
static void printparam(char *, char *, void *, int);

static void vendoroptions(char *, unsigned char *, int);
static void vendorclass(char *, unsigned char *, int);

struct {
	int value;
	char *name;
	void (*func)(char *, char *, void *, int);
} params[] = {
#ifdef B_ACPI_SMAP
	{ B_ACPI_SMAP,		"acpi_smap",		printacpismap, },
#endif
#ifdef B_AUTODEBUG
	{ B_AUTODEBUG,		"autodebug",		printautodebug, },
#endif
#ifdef B_BIOSGEOM
	{ B_BIOSGEOM,		"biosgeom",		printbiosgeom, },
#endif
#ifdef B_BIOSINFO
	{ B_BIOSINFO,		"biosinfo",		printbiosinfo, },
#endif
#ifdef B_BIOSPM
	{ B_BIOSPM,		"biospm",		printbiospm, },
#endif
#ifdef B_BOOTPVENDOR
	{ B_BOOTPVENDOR,	"bootpvendor",		printbootp, },
#endif
#ifdef B_BSDGEOM
	{ B_BSDGEOM,		"bsdgeom",		printbsdgeom, },
#endif
#ifdef B_CONSOLE
	{ B_CONSOLE,		"console",		printconsole, },
#endif
#ifdef B_DEVSPEC
	{ B_DEVSPEC,		"devspec",		printdevspec, },
#endif
#ifdef B_HOWTO
	{ B_HOWTO,		"howto",		printhowto, },
#endif
#ifdef B_IFCONFIG
	{ B_IFCONFIG,		"ifconfig",		printifconfig, },
#endif
#ifdef B_INTEL_MP
	{ B_INTEL_MP,		"intel_mp",		printparam, },
#endif
#ifdef B_ISAPARAM
	{ B_ISAPARAM,		"isaparam",		printisaparam, },
#endif
#ifdef B_KDEBUG
	{ B_KDEBUG,		"kdebug",		printkdebug, },
#endif
#ifdef B_KERNSPACE
	{ B_KERNSPACE,		"kernspace",		printkernspace, },
#endif
#ifdef B_MEM
	{ B_MEM,		"mem",			printmem, },
#endif
#ifdef B_MEMFS
	{ B_MEMFS,		"memfs",		printmem, },
#endif
#ifdef B_NFSPARAMS
	{ B_NFSPARAMS,		"nfsparams",		printnfs, },
#endif
#ifdef B_PARAMFOR
	{ B_PARAMFOR,		"paramfor",		printparamfor, },
#endif
#ifdef B_PNPID
	{ B_PNPID,		"pnpid",		printpnpid, },
#endif
#ifdef B_REALBOOTDEV
	{ B_REALBOOTDEV,	"realbootdev",		printrealbootdev, },
#endif
#ifdef B_ROOTTYPE
	{ B_ROOTTYPE,		"roottype",		printroottype, },
#endif
#ifdef B_SPEED
	{ B_SPEED,		"speed",		printparam, },
#endif
};

int nparams = sizeof(params) / sizeof(params[0]);

int
main(int ac, char **av)
{
	int i;

	if (ac <= 1) {
		for (i = 0; i < nparams; ++i) {
			size_t len;
			void *data = getdata(params[i].value, 0, &len);
			if (data) {
				params[i].func(params[i].name, "", data, len);
				free(data);
			}
		}
		exit (0);
	}
	while (*++av) {
		char *tail = strchr(*av, '.');

		if (tail)
			*tail = '\0';
		for (i = 0; i < nparams; ++i)
			if (strcmp(*av, params[i].name) == 0)
				break;
		if (tail)
			*tail++ = '.';
		else
			tail = "";
		if (i == nparams)
			warn("%s: no such parameter", *av);
		else {
			size_t len;
			void *data = getdata(params[i].value, 0, &len);
			if (data) {
				params[i].func(params[i].name, "", data, len);
				free(data);
			}
		}
	}

	return (0);
}

static void
printparam(char *name, char *tail, void *data, int len)
{
	u_char *s;
	int i;

	if (*tail) {
		warnx("%s: not supported", name);
		return;
	}

	for (i = len, s = data; i != 0 && isprint(*s); --i, ++s)
		;
	if (i == 0 || i == 1 && *s == '\0') {
		printf("%s: \"%.*s\"\n", name, len, (char *)data);
		return;
	}
	if (len == sizeof(int)) {
		printf("%s: %#08x (%d)\n", name, *(int *)data, *(int *)data);
		return;
	}
	printf("%s:\n", name);
	dump_mem(data, len);
	printf("\n");
}

/* DHCP Option names, formats and codes, from RFC1533.

   Format codes:

   e - end of data
   I - IP address
   l - 32-bit signed integer
   L - 32-bit unsigned integer
   s - 16-bit signed integer
   S - 16-bit unsigned integer
   b - 8-bit signed integer
   B - 8-bit unsigned integer
   t - ASCII text
   f - flag (true or false)
   A - array of whatever precedes (e.g., IA means array of IP addresses)
*/

struct {
	char	*name;
	char	*format;
	void	(*func)(char *, unsigned char *, int);
} dhcp_options[256] = {
	{ "pad", "" },
	{ "subnet-mask", "I" },
	{ "time-offset", "l" },
	{ "routers", "IA" },
	{ "time-servers", "IA" },
	{ "ien116-name-servers", "IA" },
	{ "domain-name-servers", "IA" },
	{ "log-servers", "IA" },
	{ "cookie-servers", "IA" },
	{ "lpr-servers", "IA" },
	{ "impress-servers", "IA" },
	{ "resource-location-servers", "IA" },
	{ "host-name", "t" },
	{ "boot-size", "S" },
	{ "merit-dump", "t" },
	{ "domain-name", "t" },
	{ "swap-server", "I" },
	{ "root-path", "t" },
	{ "extensions-path", "t" },
	{ "ip-forwarding", "f" },
	{ "non-local-source-routing", "f" },
	{ "policy-filter", "IIA" },
	{ "max-dgram-reassembly", "S" },
	{ "default-ip-ttl", "B" },
	{ "path-mtu-aging-timeout", "L" },
	{ "path-mtu-plateau-table", "SA" },
	{ "interface-mtu", "S" },
	{ "all-subnets-local", "f" },
	{ "broadcast-address", "I" },
	{ "perform-mask-discovery", "f" },
	{ "mask-supplier", "f" },
	{ "router-discovery", "f" },
	{ "router-solicitation-address", "I" },
	{ "static-routes", "IIA" },
	{ "trailer-encapsulation", "f" },
	{ "arp-cache-timeout", "L" },
	{ "ieee802-3-encapsulation", "f" },
	{ "default-tcp-ttl", "B" },
	{ "tcp-keepalive-interval", "L" },
	{ "tcp-keepalive-garbage", "f" },
	{ "nis-domain", "t" },
	{ "nis-servers", "IA" },
	{ "ntp-servers", "IA" },
	{ "vendor-encapsulated-options", "",  vendoroptions },
	{ "netbios-name-servers", "IA" },
	{ "netbios-dd-server", "IA" },
	{ "netbios-node-type", "B" },
	{ "netbios-scope", "t" },
	{ "font-servers", "IA" },
	{ "x-display-manager", "IA" },
	{ "dhcp-requested-address", "I" },
	{ "dhcp-lease-time", "L" },
	{ "dhcp-option-overload", "B" },
	{ "dhcp-message-type", "B" },
	{ "dhcp-server-identifier", "I" },
	{ "dhcp-parameter-request-list", "BA" },
	{ "dhcp-message", "t" },
	{ "dhcp-max-message-size", "S" },
	{ "dhcp-renewal-time", "L" },
	{ "dhcp-rebinding-time", "L" },
	{ "dhcp-class-identifier", "t", vendorclass },
	{ "dhcp-client-identifier", "t" },
	{ "option-62", "X" },
	{ "option-63", "X" },
	{ "nisplus-domain", "t" },
	{ "nisplus-servers", "IA" },
	{ "tftp-server-name", "t" },
	{ "bootfile-name", "t" },
	{ "mobile-ip-home-agent", "IA" },
	{ "smtp-server", "IA" },
	{ "pop-server", "IA" },
	{ "nntp-server", "IA" },
	{ "www-server", "IA" },
	{ "finger-server", "IA" },
	{ "irc-server", "IA" },
	{ "streettalk-server", "IA" },
	{ "streettalk-directory-assistance-server", "IA" },
	{ "user-class", "t" },
	{ "option-78", "X" },
	{ "option-79", "X" },
	{ "option-80", "X" },
	{ "option-81", "X" },
	{ "option-82", "X" },
	{ "option-83", "X" },
	{ "option-84", "X" },
	{ "nds-servers", "IA" },
	{ "nds-tree-name", "X" },
	{ "nds-context", "X" },
	{ "option-88", "X" },
	{ "option-89", "X" },
	{ "option-90", "X" },
	{ "option-91", "X" },
	{ "option-92", "X" },
	{ "option-93", "X" },
	{ "option-94", "X" },
	{ "option-95", "X" },
	{ "option-96", "X" },
	{ "option-97", "X" },
	{ "option-98", "X" },
	{ "option-99", "X" },
	{ "option-100", "X" },
	{ "option-101", "X" },
	{ "option-102", "X" },
	{ "option-103", "X" },
	{ "option-104", "X" },
	{ "option-105", "X" },
	{ "option-106", "X" },
	{ "option-107", "X" },
	{ "option-108", "X" },
	{ "option-109", "X" },
	{ "option-110", "X" },
	{ "option-111", "X" },
	{ "option-112", "X" },
	{ "option-113", "X" },
	{ "option-114", "X" },
	{ "option-115", "X" },
	{ "option-116", "X" },
	{ "option-117", "X" },
	{ "option-118", "X" },
	{ "option-119", "X" },
	{ "option-120", "X" },
	{ "option-121", "X" },
	{ "option-122", "X" },
	{ "option-123", "X" },
	{ "option-124", "X" },
	{ "option-125", "X" },
	{ "option-126", "X" },
	{ "option-127", "X" },
	{ "option-128", "X" },
	{ "option-129", "X" },
	{ "option-130", "X" },
	{ "option-131", "X" },
	{ "option-132", "X" },
	{ "option-133", "X" },
	{ "option-134", "X" },
	{ "option-135", "X" },
	{ "option-136", "X" },
	{ "option-137", "X" },
	{ "option-138", "X" },
	{ "option-139", "X" },
	{ "option-140", "X" },
	{ "option-141", "X" },
	{ "option-142", "X" },
	{ "option-143", "X" },
	{ "option-144", "X" },
	{ "option-145", "X" },
	{ "option-146", "X" },
	{ "option-147", "X" },
	{ "option-148", "X" },
	{ "option-149", "X" },
	{ "option-150", "X" },
	{ "option-151", "X" },
	{ "option-152", "X" },
	{ "option-153", "X" },
	{ "option-154", "X" },
	{ "option-155", "X" },
	{ "option-156", "X" },
	{ "option-157", "X" },
	{ "option-158", "X" },
	{ "option-159", "X" },
	{ "option-160", "X" },
	{ "option-161", "X" },
	{ "option-162", "X" },
	{ "option-163", "X" },
	{ "option-164", "X" },
	{ "option-165", "X" },
	{ "option-166", "X" },
	{ "option-167", "X" },
	{ "option-168", "X" },
	{ "option-169", "X" },
	{ "option-170", "X" },
	{ "option-171", "X" },
	{ "option-172", "X" },
	{ "option-173", "X" },
	{ "option-174", "X" },
	{ "option-175", "X" },
	{ "option-176", "X" },
	{ "option-177", "X" },
	{ "option-178", "X" },
	{ "option-179", "X" },
	{ "option-180", "X" },
	{ "option-181", "X" },
	{ "option-182", "X" },
	{ "option-183", "X" },
	{ "option-184", "X" },
	{ "option-185", "X" },
	{ "option-186", "X" },
	{ "option-187", "X" },
	{ "option-188", "X" },
	{ "option-189", "X" },
	{ "option-190", "X" },
	{ "option-191", "X" },
	{ "option-192", "X" },
	{ "option-193", "X" },
	{ "option-194", "X" },
	{ "option-195", "X" },
	{ "option-196", "X" },
	{ "option-197", "X" },
	{ "option-198", "X" },
	{ "option-199", "X" },
	{ "option-200", "X" },
	{ "option-201", "X" },
	{ "option-202", "X" },
	{ "option-203", "X" },
	{ "option-204", "X" },
	{ "option-205", "X" },
	{ "option-206", "X" },
	{ "option-207", "X" },
	{ "option-208", "X" },
	{ "option-209", "X" },
	{ "option-210", "X" },
	{ "option-211", "X" },
	{ "option-212", "X" },
	{ "option-213", "X" },
	{ "option-214", "X" },
	{ "option-215", "X" },
	{ "option-216", "X" },
	{ "option-217", "X" },
	{ "option-218", "X" },
	{ "option-219", "X" },
	{ "option-220", "X" },
	{ "option-221", "X" },
	{ "option-222", "X" },
	{ "option-223", "X" },
	{ "option-224", "X" },
	{ "option-225", "X" },
	{ "option-226", "X" },
	{ "option-227", "X" },
	{ "option-228", "X" },
	{ "option-229", "X" },
	{ "option-230", "X" },
	{ "option-231", "X" },
	{ "option-232", "X" },
	{ "option-233", "X" },
	{ "option-234", "X" },
	{ "option-235", "X" },
	{ "option-236", "X" },
	{ "option-237", "X" },
	{ "option-238", "X" },
	{ "option-239", "X" },
	{ "option-240", "X" },
	{ "option-241", "X" },
	{ "option-242", "X" },
	{ "option-243", "X" },
	{ "option-244", "X" },
	{ "option-245", "X" },
	{ "option-246", "X" },
	{ "option-247", "X" },
	{ "option-248", "X" },
	{ "option-249", "X" },
	{ "option-250", "X" },
	{ "option-251", "X" },
	{ "option-252", "X" },
	{ "option-253", "X" },
	{ "option-254", "X" },
	{ "option-end", "" },
};

#ifdef	B_AUTODEBUG
static void
printautodebug(char *name, char *tail, void *data, int len)
{
	int ival;
	char *sep;

	if (len < (int)sizeof(int)) {
		warn("%s truncated", name);
		return;
	}
	ival = *(int *)data;
	printf("%s=", name);
	if (ival == 0) {
		printf("0\n");
		return;
	}

	sep = "";
#define	APRINT(flag, value) \
	if (ival & (flag)) { \
		printf("%s%s", sep, (value)); \
		sep = ","; \
	}
	APRINT(AC_DEBUG, "debug");
	APRINT(AC_ASK, "ask");
	APRINT(AC_QUIET, "quiet");
	APRINT(AC_VERBOSE, "verbose");
	APRINT(AC_PAGE, "page");
#undef	APRINT
	printf("\n");
}
#endif

#ifdef	B_BIOSGEOM
static void
printbiosgeom(char *name, char *tail, void *data, int len)
{
	struct biosgeom *bg;
	char *sep;

	if (len < (int)sizeof(*bg)) {
		warn("%s truncated", name);
		return;
	}
	bg = data;
	printf("%s.cylinders=%d\n", name, bg->ncylinders);
	printf("%s.heads=%d\n", name, bg->ntracks);
	printf("%s.eflag=%#x\n", name, bg->eflag);
	printf("%s.esectors=%d\n", name, bg->esectors);
	printf("%s.wpcom=%#x,%#x\n", name, bg->wpcom[0], bg->wpcom[1]);
	printf("%s.ctl=%#x\n", name, bg->ctl);
	printf("%s.ecyl=%#x,%#x\n", name, bg->ecyl[0], bg->ecyl[1]);
	printf("%s.etracks=%d\n", name, bg->etracks);
	printf("%s.lzone=%#x,%#x\n", name, bg->lzone[0], bg->lzone[1]);
	printf("%s.sectorspertrk=%d\n", name, bg->nsectors);
	printf("%s.flags=", name);
	sep = "";
#define	APRINT(flag, value) \
	if (bg->flags & (flag)) { \
		printf("%s%s", sep, (value)); \
		sep = ","; \
	}
	APRINT(BIOSGEOM_PRESENT, "present");
	APRINT(BIOSGEOM_MAPPED, "mapped");
	APRINT(BIOSGEOM_EIDE, "eide");
#undef	APRINT
	printf("\n");
}
#endif

#ifdef	B_BIOSINFO
static void
printbiosinfo(char *name, char *tail, void *data, int len)
{
	struct biosinfo *bi;
	char *sep;
	
	if (len < (int)sizeof(*bi)) {
		warn("%s truncated", name);
		return;
	}
	bi = data;
	printf("%s.basemem=%#08x\n", name, bi->basemem);
	printf("%s.extmem=%#08x\n", name, bi->extmem);
	printf("%s.kbflags=", name);
	sep = "";
#define	APRINT(flag, value) \
	if (bi->kbflags & (flag)) { \
		printf("%s%s", sep, (value)); \
		sep = ","; \
	}
	APRINT(BIOS_KB_NUMLOCK, "numlock");
	APRINT(BIOS_KB_ALT, "alt");
	APRINT(BIOS_KB_CTRL, "ctrl");
	APRINT(BIOS_KB_LSHIFT, "lshift");
	APRINT(BIOS_KB_RSHIFT, "rshift");
#undef	APRINT
	printf("\n");
}
#endif

#ifdef	B_BIOSPM
static void
printbiospm(char *name, char *tail, void *data, int len)
{
	struct biospmparam *pm;
	
	if (len < (int)sizeof(*pm)) {
		warn("%s truncated", name);
		return;
	}
	pm = data;
	printf("%s.magic=%#x\n", name, pm->pm_magic);
	printf("%s.err=%#x\n", name, pm->pm_err);
	printf("%s.flags=%#x\n", name, pm->pm_flags);
	printf("%s.version=%#x\n", name, pm->pm_version);
	printf("%s.32cseg=%#x\n", name, pm->pm_32cseg);
	printf("%s.32coff=%#lx\n", name, pm->pm_32coff);
	printf("%s.16cseg=%#x\n", name, pm->pm_16cseg);
	printf("%s.16coff=%#x\n", name, pm->pm_16dseg);

}
#endif

#ifdef	B_ACPI_SMAP
static void
printacpismap(char *name, char *tail, void *data, int len)
{
	struct acpi_smap *sp, *slp;
	u_int i;

	for (i = 0, sp = data, slp = data + len; sp < slp; i++, sp++) {
		printf("%s.%u.type=", name, i);
		switch (sp->sm_type) {
		case ACPI_SMAP_MEMORY:
			printf("memory");
			break;
		case ACPI_SMAP_RESV:
			printf("reserved");
			break;
		case ACPI_SMAP_ACPI:
			printf("acpi");
			break;
		case ACPI_SMAP_NVS:
			printf("nvs");
			break;
		default:
			printf("%d", sp->sm_type);
			break;
		}
		printf("\n");
		printf("%s.%u.base=%016qx\n", name, i, sp->sm_base);
		printf("%s.%u.type=%016qx\n", name, i, sp->sm_length);
	}
}
#endif	/* B_ACPI_SMAP */

static void
printbootp(char *name, char *tail, void *data, int len)
{
	u_char *d, *p = data, *ep;
	u_char tag, size, s;
	char *format;
	char fullname[256];
	char fakeformat[256];
	char visstr[256*4];
	u_long n;
	char *space;

	if (p[0] != 0x63 || p[1] != 0x82 || p[2] != 0x53 || p[3] != 0x63) {
		printparam(name, tail, data, len);
		return;
	}
	ep = p + len;
	for (p += 4; p < ep; p += size) {
		tag = *p++;
		s = size = *p++;
		d = p;

		sprintf(fullname, "%s.%s", name, dhcp_options[tag].name);
		if (dhcp_options[tag].func)
			dhcp_options[tag].func(fullname, p, size);
		format = dhcp_options[tag].format;
		if (*format == '\0')
			continue;
		printf("%s=", fullname);
		space = "";
		while (s > 0 && *format) {
			switch (*format) {
			case 'I':
				printf("%s%d.%d.%d.%d", space, d[0], d[1], d[2], d[3]);
				space = " ";
				d += 4;
				s -= 4;
				break;

			case 'l':
				n = 0;
				n |= *d++;
				n <<= 8;
				n |= *d++;
				n <<= 8;
				n |= *d++;
				n <<= 8;
				n |= *d++;
				s -= 4;

				printf("%s%d", space, (int)n);
				space = " ";
				break;

			case 'L':
				n = 0;
				n |= *d++;
				n <<= 8;
				n |= *d++;
				n <<= 8;
				n |= *d++;
				n <<= 8;
				n |= *d++;
				s -= 4;

				printf("%s%u", space, (int)n);
				space = " ";
				break;

			case 's':
				n = 0;
				n |= *d++;
				n <<= 8;
				n |= *d++;
				s -= 2;

				printf("%s%d", space, (short)n);
				space = " ";
				break;

			case 'S':
				n = 0;
				n |= *d++;
				n <<= 8;
				n |= *d++;
				s -= 2;

				printf("%s%u", space, (u_short)n);
				space = " ";
				break;

			case 'b':
				printf("%s%d", space, *(char *)d);
				space = " ";
				++d;
				--s;
				break;

			case 'B':
				printf("%s%u", space, *(char *)d);
				space = " ";
				++d;
				--s;
				break;

			case 't':
				printf("%s%.*s", space, s, d);
				space = " ";
				while (*d && s > 0) {
					--s;
					++d;
				}
				if (s) {
					--s;
					++d;
				}
				break;

			case 'f':
				printf("%s%s", space, *d ? "True" : "False");
				space = " ";
				--s;
				++d;
				break;

			case 'A':
				memset(fakeformat, format[-1], s);
				format = fakeformat;
				break;

			default:
				strvisx(visstr, d, s, VIS_WHITE);
				printf("%s%s", space, visstr);
				s = 0;
				break;
			}
		}
		printf("\n");
	}
}

static int vendor_class = 0;
#define	VENDOR_BSDi	1

static void
vendoroptions(char *name, unsigned char *p, int len)
{
	char visstr[256*4];
	unsigned char *ep;
	int tag, size;

	ep = p + len;

	switch (vendor_class) {
	case VENDOR_BSDi:
		for (; p < ep; p += size) {
			tag = *p++;
			size = *p++;
			switch (tag) {
			case 1:
				printf("%s.boot.define=%.*s\n", name, size,p);
				break;
			case 2:
				printf("%s.boot.default=%.*s\n", name, size,p);
				break;
			default:
				strvisx(visstr, p, size, VIS_WHITE);
				printf("%s.%d=%s\n", name, tag, visstr);
				break;
			}
		}
		break;

	default:
		for (; p < ep; p += size) {
			tag = *p++;
			size = *p++;
			strvisx(visstr, p, size, VIS_WHITE);
			printf("%s.%d=%s\n", name, tag, visstr);
		}
		break;
	}
}

static void
vendorclass(char *name, unsigned char *op, int len)
{
	vendor_class = 0;
	if (len == 4 && memcmp(op, "BSDi", 4) == 0)
		vendor_class = VENDOR_BSDi;
}

#ifdef	B_BSDGEOM
static void
printbsdgeom(char *name, char *tail, void *data, int len)
{
	struct bsdgeom *bd;

	if (len < (int)sizeof(*bd)) {
		warn("%s truncated", name);
		return;
	}
	bd = data;
	printf("%s.unit=%d\n", name, bd->unit);
	printf("%s.ncylinders=%ld\n", name, bd->ncylinders);
	printf("%s.ntracks=%ld\n", name, bd->ntracks);
	printf("%s.nsectors=%ld\n", name, bd->nsectors);
	printf("%s.startsec=%d\n", name, (int)bd->bsd_startsec);
}
#endif

#ifdef	B_CONSOLE
static void
printconsole(char *name, char *tail, void *data, int len)
{
	struct bootcons *bc;

	if (len < (int)sizeof(*bc)) {
		warn("%s truncated", name);
		return;
	}
	bc = data;

	printf("%s.type=", name);
	switch (bc->type) {
	case BOOTCONS_KBDISP:
		printf("conskbd");
		break;
	case BOOTCONS_COM:
		printf("serial");
		break;
	}
	printf("\n");
	printf("%s.unit=%d\n", name, bc->unit);
	switch (bc->type) {
	case BOOTCONS_COM:
		printf("%s.port=%#x\n", name, bc->val0);
		printf("%s.baudrate=%d\n", name, bc->val1);
		break;
	default:
		printf("%s.val0=%#x\n", name, bc->val0);
		printf("%s.val1=%#x\n", name, bc->val1);
		break;
	}
}
#endif

#ifdef	B_DEVSPEC
static char *locators[] = {
	"port",
	"iosize",
	"maddr",
	"msize",
	"irq",
	"drq",
	"bustype",
	"flags"
};
static u_int devspec_index;

static void
printdevspec(char *name, char *tail, void *data, int len)
{
	struct boot_devspec *ds;
	int i;
	int idx;

	if (len < (int)sizeof(*ds)) {
		warn("%s truncated", name);
		return;
	}
	ds = data;
	idx = devspec_index++;
	printf("%s.%u.driver=%s\n", name, idx, ds->ds_driver);
	printf("%s.%u.unit=%d\n", name, idx, ds->ds_unit);
	printf("%s.%u.mask=%#x\n", name, idx, ds->ds_validmask);
	for (i = 0; i <= DSLOC_FLAGS; i++) {
		if ((ds->ds_validmask & 1 << i) == 0)
			continue;
		printf("%s.%u.%s=", name, idx, locators[i]);
		switch (i) {
		case 6:	/* Bus type */
			switch(ds->ds_loc[i]) {
#ifdef BUS_ANY
			case BUS_ANY:
				printf("any");
				break;
#endif
#ifdef BUS_ISA
			case BUS_ISA:
				printf("isa");
				break;
#endif
#ifdef BUS_EISA
			case BUS_EISA:
				printf("eisa");
				break;
#endif
#ifdef BUS_MCA
			case BUS_MCA:
				printf("mca");
				break;
#endif
#ifdef BUS_PCI
			case BUS_PCI:
				printf("pci");
				break;
#endif
#ifdef BUS_PCMCIA
			case BUS_PCMCIA:
				printf("pcmcia");
				break;
#endif
			default:
				printf("%d", ds->ds_loc[i]);
				break;
			}
			break;
		default:
			printf("%#x", ds->ds_loc[i]);
			break;
		}
		printf("\n");
	}
}
#endif

#ifdef	B_HOWTO
static void
printhowto(char *name, char *tail, void *data, int len)
{
	int ival;

	if (len < (int)sizeof(int)) {
		warn("%s truncated", name);
		return;
	}
	ival = *(int *)data;

	sep = "";
#define	APRINT(flag, value) \
	if (ival & (flag)) { \
		printf("%s%s", sep, (value)); \
		sep = ","; \
	}
	APRINT(RB_ASKNAME, "askname");
	APRINT(RB_SINGLE, "single");
	APRINT(RB_NOSYNC, "nosync");
	APRINT(RB_HALT, "halt");
	APRINT(RB_INITNAME, "initname");
	APRINT(RB_DFLTROOT, "dfltroot");
	APRINT(RB_KDB, "kdb");
	APRINT(RB_RWROOT, "rwroot");
	APRINT(RB_DUMP, "dump");
	APRINT(RB_MINIROOT, "miniroot");
	APRINT(RB_POWEROFF, "poweroff");
#undef	APRINT
	printf("\n");
}
#endif

#ifdef	B_IFCONFIG
static u_int ifconfig_index;

static char *
printifconfig_addr(struct sockaddr *sa, char *buf)
{
	char *cp, *dp, *lp;

	switch (sa->sa_family) {
	case AF_LINK:
		sprintf(buf, "%s", link_ntoa((struct sockaddr_dl *)sa));
		break;
	case AF_INET:
	case AF_INET6:
		if (getnameinfo(sa, sa->sa_len, buf, NI_MAXHOST,
		    NULL, 0, NI_NUMERICHOST) == 0)
			break;
		/* FALL THROUGH */
	default:
		dp = buf;
		dp += sprintf(dp, "%u,%u", sa->sa_len, sa->sa_family);
		for (cp = sa->sa_data, lp = sa->sa_data + sa->sa_len;
		     cp < lp; lp++)
			dp += sprintf(dp, ",%02x", *cp);
		break;
	}

	return (buf);
}

static void
printifconfig(char *name, char *tail, void *data, int len)
{
	struct bootparam_if *bif;
	int idx;
	char buf[MAX(NI_MAXHOST,LINE_MAX)];

	if (len < (int)sizeof(*bif)) {
		warn("%s truncated", name);
		return;
	}
	bif = data;
	idx = ifconfig_index++;

	printf("%s.%u.media=%08lx\n", name, idx, bif->bif_media);
	printf("%s.%u.flags=%08lx\n", name, idx, bif->bif_flags);
	if (bif_link(bif) != NULL)
		printf("%s.%u.link=%s\n", name, idx,
		    printifconfig_addr((struct sockaddr *)bif_link(bif), buf));
	if (bif_addr(bif, void) != NULL)
		printf("%s.%u.addr=%s\n", name, idx, 
		    printifconfig_addr(bif_addr(bif, struct sockaddr), buf));
	if (bif_mask(bif, void) != NULL)
		printf("%s.%u.mask=%s\n", name, idx,
		    printifconfig_addr(bif_mask(bif, struct sockaddr), buf));
	if (bif_broad(bif, void) != NULL)
		printf("%s.%u.broad=%s\n", name, idx,
		    printifconfig_addr(bif_broad(bif, struct sockaddr), buf));
}
#endif

#ifdef	B_ISAPARAM
static void
printisaparam(char *name, char *tail, void *data, int len)
{
	struct isaparam *isa;
	int bit;
	int i;
	int idx;
	int nst;
	int oldi;
	int st;
	char *sep;

	if (len < (int)sizeof(int)) {
		warn("%s truncated", name);
		return;
	}
	isa = data;

	printf("%s.reserved.irqs=", name);
	sep = "";
	for (i = 0; i < 16; i++)
		if (isa->isa_irqmap & 1 << i) {
			printf("%s%d", sep, i + 1);
			sep=",";
		}
	printf("\n");

	printf("%s.reserved.ports=", name);
	st = 0;
	sep = "";
	oldi = -1;
	for (i = 0; i < ISA_NPORT_CHECK; i++) {
		idx = i >> 3;
		bit = 1 << (i & 0x7);
		nst = (isa->isa_portmap[idx] & bit) != 0;
		if (st ^ nst) {
			if (st == 0) {
				printf("%s%#x-", sep, i);
				sep = ",";
				oldi = i;
			} else
				if (i - 1 != oldi) 
					printf("%#x", i - 1);
			st = nst;
		}
	}
	if (st)
		printf("-%#x", i - 1);
	printf("\n");

	printf("%s.reserved.iomem=", name);
	st = 0;
	sep = "";
	for (i = 0; i < IOM_NPAGE; i++) {
		idx = i >> 3;
		bit = 1 << (i & 0x7);
		nst = (isa->isa_memmap[idx] & bit) != 0;
		if (st ^ nst) {
			if (st == 0) {
				printf("%s%#x", sep,
				    IOM_BEGIN + (i * IOM_PGSZ));
				sep = ",";
				oldi = i;
			} else
				if (i - 1 != oldi)
					printf("-%#x",
					    IOM_BEGIN + 
					    (i * IOM_PGSZ) - 1);
			st = nst;
		}
	}
	if (st)
		printf("-%#x", IOM_BEGIN + i * IOM_PGSZ - 1);
	printf("\n");

	printf("%s.pccard.iowait=%d\n", name, isa->pccard_iowait);
}
#endif

#ifdef	B_KDEBUG
static void
printkdebug(char *name, char *tail, void *data, int len)
{

	if (len < (int)sizeof(int)) {
		warn("%s truncated", name);
		return;
	}
	printf("%s=%#x\n", name, *(int *)data);
}
#endif

#ifdef	B_KERNSPACE
static void
printkernspace(char *name, char *tail, void *data, int len)
{

	if (len < (int)sizeof(int)) {
		warn("%s truncated", name);
		return;
	}
	printf("%s=%08x\n", name, *(u_int *)data);
}
#endif

#if	defined(B_MEM) || defined(B_MEMFS)
static void
printmem(char *name, char *tail, void *data, int len)
{
	struct boot_mem *mem;

	if (len < (int)sizeof(*mem)) {
		warn("%s truncated", name);
		return;
	}

	mem = data;
	printf("%s.base=%08x\n", name, mem->membase);
	printf("%s.len=%08x\n", name, mem->memlen);
}
#endif

#ifdef	B_NFSPARAMS
static void
printnfs(char *name, char *tail, void *data, int len)
{
	struct nfs_diskless *nd = data;
	struct tm *tm;
	char buf[32];

	u_char *u;

	if (len < (int)sizeof(*nd)) {
		warn("%s truncated", name);
		return;
	}
	printf("%s.myif.ifname=%s\n", name, nd->myif.ifra_name);

	u = (u_char *)&nd->myif.ifra_addr.sin_addr;
	printf("%s.myif.addr=%d.%d.%d.%d\n", name, u[0], u[1], u[2], u[3]);

	u = (u_char *)&nd->myif.ifra_mask.sin_addr;
	printf("%s.myif.mask=%d.%d.%d.%d\n", name, u[0], u[1], u[2], u[3]);

	u = (u_char *)&nd->myif.ifra_broadaddr.sin_addr;
	printf("%s.myif.broadaddr=%d.%d.%d.%d\n", name, u[0], u[1], u[2], u[3]);

	u = (u_char *)&nd->mygateway.sin_addr;
	printf("%s.mygateway=%d.%d.%d.%d\n", name, u[0], u[1], u[2], u[3]);

	u = (u_char *)&nd->swap_saddr.sin_addr;
	printf("%s.swap_saddr=%d.%d.%d.%d\n", name, u[0], u[1], u[2], u[3]);
	printf("%s.swap_path=%.*s\n", name, MNAMELEN, nd->swap_path);
	printf("%s.swap_nblks=%d\n", name, nd->swap_nblks);

	u = (u_char *)&nd->root_saddr.sin_addr;
	printf("%s.root_saddr=%d.%d.%d.%d\n", name, u[0], u[1], u[2], u[3]);
	printf("%s.root_path=%.*s\n", name, MNAMELEN, nd->root_path);
	
	tm = localtime(&nd->root_time);
	strftime(buf, sizeof(buf), "%Y.%m.%d.%H.%M.%S", tm);
	printf("%s.root_time=%s\n", name, buf);

	printf("%s.hostname=%.*s\n", name, MAXHOSTNAMELEN, nd->my_hostnam);

#if 0
        int             swap_sotype;            /* Socket type for swap file */
        size_t          swap_fhsize;            /* Size of file handle */
        u_char          swap_fh[NFSX_V3FHMAX];  /* Swap file's file handle */
        struct ucred    swap_ucred;             /* Swap credentials */
        int             root_sotype;            /* Socket type for root fs */
        size_t          root_fhsize;            /* Size of root file handle */
        u_char          root_fh[NFSX_V3FHMAX];  /* File handle of root dir */
#endif
}
#endif

#ifdef	B_PARAMFOR
static u_int paramfor_index;

static void
printparamfor(char *name, char *tail, void *data, int len)
{
	struct boot_paramfor *bp;
	u_int idx;
	
	if (len < (int)sizeof(*bp)) {
		warn("%s truncated", name);
		return;
	}
	bp = data;
	idx = paramfor_index++;
	/* XXX - parse /etc/boot.define for symbolic names */
	printf("%s.%u.name=%.*s\n", name, idx,
	    (int)sizeof(bp->pf_name), bp->pf_name);
	printf("%s.%u.field=%u\n", name, idx, bp->pf_field);
	printf("%s.%u.flags=%#x\n", name, idx, bp->pf_flags);
}
#endif

#ifdef	B_PNPID
static u_int pnpid_index;

static void
printpnpid(char *name, char *tail, void *data, int len)
{
	struct boot_pnpid *pi;
	int idx;

	if (len < (int)sizeof(*pi)) {
		warn("%s truncated", name);
		return;
	}
	pi = data;
	idx = pnpid_index++;
	printf("%s.%u.driver=%.*s\n", name, idx, 
	    (int)sizeof(pi->pi_devname), pi->pi_devname);
	printf("%s.%u.id=%.*s\n", name, idx,
	    (int)sizeof(pi->pi_id), pi->pi_id);
}
#endif

#ifdef	B_REALBOOTDEV
static void
printrealbootdev(char *name, char *tail, void *data, int len)
{
	int ival;

	if (len < (int)sizeof(int)) {
		warn("%s truncated", name);
		return;
	}
	ival = *(int *)data;
	printf("%s=%u,%u,%u\n",
	    name,
	    (ival >> B_TYPESHIFT) & B_TYPEMASK,
	    (ival >> B_PARTITIONSHIFT) & B_PARTITIONMASK,
	    (ival >> B_UNITSHIFT) & B_UNITMASK);
}
#endif

#ifdef	B_ROOTTYPE
static void
printroottype(char *name, char *tail, void *data, int len)
{
	int ival;

	if (len < (int)sizeof(int)) {
		warn("%s truncated", name);
		return;
	}
	ival = *(int *)data;
	switch (ival) {
	case ROOT_LOCAL:
		printf("%s=local\n", name);
		break;
	case ROOT_NFS:
		printf("%s=nfs\n", name);
		break;
	case ROOT_MEMFS:
		printf("%s=memfs\n", name);
		break;
	default:
		printf("%s=%d\n", name, ival);
		break;
	}
}
#endif

static void *
getdata(int what, int version, size_t *len)
{
	int mib[] = { CTL_KERN, KERN_BOOTPARAMS, what, version };
	void *buf;
	size_t s;

	s = 0;
	if (sysctl(mib, 4, NULL, &s, NULL, 0))
		return (NULL);
	if ((buf = malloc(s)) == NULL)
		return (NULL);
	if (sysctl(mib, 4, buf, &s, NULL, 0)) {
		free(buf);
		return (NULL);
	}
	*len = s;
	return (buf);
}

static char hexdigits[] = "0123456789ABCDEF";
#define	d2h(x)	hexdigits[(x) & 0xf]

static char *
bintohex(unsigned int v)
{
	char *p;
	static char buf[9];

	p = buf + 9;
	*--p = '\0';
	while (v) {
		*--p = d2h(v);
		v >>= 4;
	}
	while (p > buf)
		*--p = '0';
	return(buf);
}

static void
dump_mem(unsigned char *cp, int len)
{
	unsigned int offset;
	unsigned char *base;
	int i;

	offset = 0;
	base = cp;

	while (len > 0) {
		printf("%s: ", bintohex(offset));
		for (i = 0; i < 16; ++i) {
			if (i == 8)
				printf(" ");
			if (i < len)
				printf("%s ", bintohex(base[i]) + 6);
			else
				printf("   ");
		}
		printf(" :");
		for (i = 0; i < 16; ++i) {
			if (i == 8)
				printf(" ");
			if (i < len) {
				if (base[i] < ' ' || base[i] > '~')
				    printf(".");
				else
				    printf("%c", base[i]);
			} else
				printf(" ");
		}
		printf("\n");
		len -= 16;
		base += 16;
		offset += 16;
	}
}
