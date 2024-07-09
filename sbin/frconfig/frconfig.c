/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI frconfig.c,v 1.4 1999/04/19 17:25:48 polk Exp
 */
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/if_llc.h>
#include <net/if_dl.h>

#include <machine/cpu.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <net/if_p2p.h>

#include <net/if_fr.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

int
getnum(char *s, int *n)
{
	char *e;

	if (s == NULL)
		return (1);
	*n = strtol(s, &e, 0);
	if (s == e || *e)
		return (1);
	return (0);
}

int
getunum(char *s, u_int32_t *n)
{
	char *e;

	if (s == NULL)
		return (1);
	*n = strtoul(s, &e, 0);
	if (s == e || *e)
		return (1);
	return (0);
}

displayroutes(int s, char *iface, int cmd)
{
	struct fr_mapreq frm;
	struct fr_map *map;
	int i, j;

	strcpy(frm.fr_name, iface);

	for (;;) {
		frm.fr_map = NULL;
		frm.fr_size = 0;

		if (ioctl(s, cmd, &frm) < 0)
			err(1, "%s: cmd", frm.fr_name);
		if ((frm.fr_map = malloc(i = frm.fr_size)) == NULL)
			err(1, NULL);
		if (ioctl(s, cmd, &frm) < 0)
			err(1, "%s: cmd", frm.fr_name);
		if (i >= frm.fr_size)
			break;
		free(frm.fr_map);
	}
	for (map = frm.fr_map; i >= sizeof(*map); i -= sizeof(*map), ++map) {
		if (map->raw) {
			printf("raw %d\n", map->dlci);
			continue;
		}
		if (map->ipaddr.s_addr == 0 && map->mask.s_addr == 0) {
			printf("dlci %d -\n", map->dlci);
			continue;
		}
		printf("dlci %d %s", map->dlci,
			inet_ntoa(map->ipaddr));
		if (map->mask.s_addr == 0xffffffff)
			printf("\n");
		else {
			for (j = 1; j < 32; ++j)
			if (map->mask.s_addr == 0xffffffff << j) {
				printf("/%d\n", 32 - j);
				break;
			}
			if (j >= 32)
				printf("/%08x\n", map->mask.s_addr);
		}
	}
}

main(int ac, char **av)
{
	struct fr_req frq;
	struct fr_rtreq frt;
	struct hostent *hp;
	int changed = 0;
	u_int32_t dlci;
	int s;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "socket(AF_INET, SOCK_DGRAM)");

	if (ac < 2) {
		fprintf(stderr, "usage: frdlci interface [cmds]\n");
		exit(1);
	}

	strcpy(frq.fr_name, av[1]);
	strcpy(frt.fr_name, av[1]);
	av += 2;
	ac -= 2;

	if (ioctl(s, FR_GCONFIG, &frq) < 0)
		err(1, "%s: FR_GCONFIG", frq.fr_name);

	if (ac == 0) {
		printf("%s\n", frq.fr_name);
		printf("type %s\n", frq.fr_config.fc_type ? "DCE" : "DTE");
		printf("lmi %s\n",
			frq.fr_config.fc_lmitype == FR_LMI_TYPE_ANSI ? "ANSI" :
			frq.fr_config.fc_lmitype == FR_LMI_TYPE_FRIF ? "ORIG" :
			frq.fr_config.fc_lmitype == FR_LMI_TYPE_CCITT ? "CCITT" :
			"unknown");
		printf("N391 %d\n", frq.fr_config.fc_nN1);
		printf("N392 %d\n", frq.fr_config.fc_nN2);
		printf("N393 %d\n", frq.fr_config.fc_nN3);
		printf("T391 %d\n", frq.fr_config.fc_nT1);
		printf("maxframe %d\n", frq.fr_config.fc_dN1);
		if (frq.fr_config.fc_fecn_used)
			printf("fecn %d %d\n", frq.fr_config.fc_fecn_k,
			    frq.fr_config.fc_fecn_n);
		else 
			printf("fecn unused\n");
		if (frq.fr_config.fc_becn_used)
			printf("becn %d %d\n", frq.fr_config.fc_becn_k,
			    frq.fr_config.fc_becn_n);
		else 
			printf("becn unused\n");
		printf("niarps %d\n", frq.fr_config.fc_max_dispatch);
		printf("discard %d\n", frq.fr_config.fc_discard_eli);
		printf("mode %s\n", (frq.fr_config.fc_mode & IFF_POINTOPOINT) ?
		    "P2P" : "MDNB");
		printf("default %d\n", frq.fr_config.fc_dlci);
		displayroutes(s, frq.fr_name, FR_GROUTE);

		exit (0);
	}

	while (*av) {
		if (strcmp(*av, "-map") == 0) {
			displayroutes(s, frq.fr_name, FR_GMAP);
			++av;
		} else
		if (strcmp(*av, "dlci") == 0) {
			char *mask;
			if (av[1] == NULL || av[2] == NULL)
				errx(1, "usage dlci index host");
                        if (getunum(av[1], &dlci))
				errx("%s: invalid dlci\n", av[1]);
			frt.map.dlci = dlci;
			if ((mask = strchr(av[2], '/')) != NULL) {
				*mask++ = '\0';
				if (getunum(mask, &frt.map.mask.s_addr))
					errx(1, "%s: invalid mask\n", mask);
				if (frt.map.mask.s_addr < 33)
					frt.map.mask.s_addr = (0xffffffff << (32 - frt.map.mask.s_addr)) & 0xffffffff;
			} else
				frt.map.mask.s_addr = 0xffffffff;
			frt.map.mask.s_addr = ntohl(frt.map.mask.s_addr);
			if ((hp = gethostbyname(av[2])) == NULL)
				errx(1, "%s: no such host\n", av[2]);
			memcpy(&frt.map.ipaddr, hp->h_addr, 4);
			frt.map.ipaddr.s_addr &= frt.map.mask.s_addr;
			if (ioctl(s, FR_SROUTE, &frt) < 0)
				err(1, "FR_SROUTE");
			av += 3;
		} else
		if (strcmp(*av, "-dlci") == 0) {
			char *mask;
			if (av[1] == NULL || av[2] == NULL)
				errx(1, "usage -dlci index host[/mask]");
                        if (getunum(av[1], &dlci))
				errx("%s: invalid dlci\n", av[1]);
			frt.map.dlci = dlci;
			if ((mask = strchr(av[2], '/')) != NULL) {
				*mask++ = '\0';
				if (getunum(mask, &frt.map.mask.s_addr))
					errx(1, "%s: invalid mask\n", mask);
				if (frt.map.mask.s_addr < 33)
					frt.map.mask.s_addr = (0xffffffff << (32 - frt.map.mask.s_addr)) & 0xffffffff;
			} else
				frt.map.mask.s_addr = 0xffffffff;
			frt.map.mask.s_addr = ntohl(frt.map.mask.s_addr);
			if ((hp = gethostbyname(av[2])) == NULL)
				errx(1, "%s: no such host\n", av[2]);
			memcpy(&frt.map.ipaddr, hp->h_addr, 4);
			frt.map.ipaddr.s_addr &= frt.map.mask.s_addr;
			if (ioctl(s, FR_CROUTE, &frt) < 0)
				err(1, "FR_CROUTE");
			av += 3;
		} else
		if (strcmp(*av, "raw") == 0) {
			if (getunum(av[1], &dlci))
				errx(1, "usage raw dlci");
			frt.map.dlci = dlci;
			frt.map.raw = 1;
			frt.map.mask.s_addr = 0;
			frt.map.ipaddr.s_addr = 0;
			if (ioctl(s, FR_SROUTE, &frt) < 0)
				err(1, "FR_SROUTE");
			av += 2;
		} else
		if (strcmp(*av, "-raw") == 0) {
			if (getunum(av[1], &dlci))
				errx(1, "usage -raw dlci");
			frt.map.dlci = dlci;
			frt.map.raw = 1;
			frt.map.mask.s_addr = 0;
			frt.map.ipaddr.s_addr = 0;
			if (ioctl(s, FR_CROUTE, &frt) < 0)
				err(1, "FR_CROUTE");
			av += 2;
		} else
		if (strcmp(*av, "type") == 0) {
			if (av[1] == NULL ||
			    strcasecmp(av[1], "DCE") &&
			    strcasecmp(av[1], "DTE"))
				errx(1, "usage type DCE|DTE");
			frq.fr_config.fc_type =
			    strcasecmp(av[1], "DCE") ? FR_DTE : FR_DCE;
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "lmi") == 0) {
			if (av[1] == NULL ||
			    strcasecmp(av[1], "ANSI") &&
			    strcasecmp(av[1], "FRIF") &&
			    strcasecmp(av[1], "ORIG") &&
			    strcasecmp(av[1], "CCITT"))
				errx(1, "usage lmi ORIG|ANSI|CCITT");
			frq.fr_config.fc_lmitype =
			    strcasecmp(av[1], "ANSI") == 0 ? FR_LMI_TYPE_ANSI :
			    strcasecmp(av[1], "CCITT") == 0 ? FR_LMI_TYPE_CCITT:
			    FR_LMI_TYPE_FRIF;
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "N391") == 0) {
			if (getnum(av[1], &frq.fr_config.fc_nN1))
				errx(1, "usage N391 number");
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "N392") == 0) {
			if (getnum(av[1], &frq.fr_config.fc_nN2))
				errx(1, "usage N392 number");
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "N393") == 0) {
			if (getnum(av[1], &frq.fr_config.fc_nN3))
				errx(1, "usage N393 number");
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "T391") == 0) {
			if (getnum(av[1], &frq.fr_config.fc_nT1))
				errx(1, "usage T391 number");
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "maxframe") == 0) {
			if (getnum(av[1], &frq.fr_config.fc_dN1))
				errx(1, "usage maxframe number");
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "fecn") == 0) {
			if (getnum(av[1], &frq.fr_config.fc_fecn_k) ||
			    getnum(av[2], &frq.fr_config.fc_fecn_n)) {
				if (av[1] == NULL ||
				    strcasecmp(av[1], "unused") != 0) {
					warnx("usage fecn unused");
					errx(1, "usage fecn k n");
				}
				frq.fr_config.fc_fecn_used = 0;
				changed = 1;
				av += 2;
			} else {
				changed = 1;
				av += 3;
			}
		} else
		if (strcmp(*av, "becn") == 0) {
			if (getnum(av[1], &frq.fr_config.fc_becn_k) ||
			    getnum(av[2], &frq.fr_config.fc_becn_n)) {
				if (av[1] == NULL ||
				    strcasecmp(av[1], "unused") != 0) {
					warnx("usage becn unused");
					errx(1, "usage becn k n");
				}
				frq.fr_config.fc_becn_used = 0;
				changed = 1;
				av += 2;
			} else {
				changed = 1;
				av += 3;
			}
		} else
		if (strcmp(*av, "niarps") == 0) {
			if (getnum(av[1], &frq.fr_config.fc_max_dispatch))
				errx(1, "usage niarps number");
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "discard") == 0) {
			if (getnum(av[1], &frq.fr_config.fc_discard_eli))
				errx(1, "usage discard number");
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "mode") == 0) {
			if (av[1] == NULL ||
			    strcasecmp(av[1], "P2P") &&
			    strcasecmp(av[1], "MDNB"))
				errx(1, "usage lmi P2P|MDNB");
			frq.fr_config.fc_mode =
			    strcasecmp(av[1], "P2P") == 0 ? IFF_POINTOPOINT : 0;
			changed = 1;
			av += 2;
		} else
		if (strcmp(*av, "default") == 0) {
			if (getunum(av[1], &dlci))
				errx(1, "usage default dlci");
			frq.fr_config.fc_dlci = dlci;
			changed = 1;
			av += 2;
		} else
		{
			errx(1, "%s: unknown command", *av);
			exit (1);
		}
	}
	if (changed && ioctl(s, FR_SCONFIG, &frq) < 0)
		err(1, "%s: FR_SCONFIG", frq.fr_name);
	exit(0);
}
