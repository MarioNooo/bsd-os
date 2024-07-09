/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI readsys.c,v 2.32 2001/10/03 17:29:57 polk Exp
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_pif.h>
#include <netinet/in.h>

#include <arpa/nameser.h>

#include <err.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "ppp.h"

#include "pbuf.h"
#include "ppp_proto.h"
#include "ppp_var.h"

/*
 * These should not be around, but are needed for compatability
 */
static char	*PN;	/* Telephone number(s) for this host. */
static char	*PF;	/* CSV of PPP flags (as in pppconfig) */
static long	TO;	/* PPP retrty timeout (1/10 sec). */
static char	*LF, *LI, *LD, *LU, *SO;

/*
 * Capability types
 */
#define STR	0
#define NUM	1
#define CONST	2
#define TIME	3
#define ADDR	4
#define CMAP	5
#define BIT	6
#define	ADDR_NULL	7
#define	ADDR_IP		8
#define	ADDR_802	9
#define	ADDR_LOCAL	10

#define P       (char **)
#define	off(x)	(((char *)&((session_t *)0)->x) - (char *)0)

static struct captab {
	char	*oldname;
	int	type;
	char	**ptr;
	char	*name;
	int	offset;
	u_long	bit;
	u_long	mask;		/* bits to leave alone */
} captab[] = {
	{ "dv",	STR,	0,	"device",	off(device), },
	{ "di",	BIT,	0,	"dialin",	off(flags),	F_DIALIN, },
	{ "do",	BIT,	0,	"dialout",	off(flags),	F_DIALOUT, },
	{ 0,	BIT,	0,	"direct",	off(flags),	F_DIRECT, },
	{ "ld",	STR,	0,	"link-down",	off(linkdown), },
	{ "lf",	STR,	0,	"link-failed",	off(linkfailed), },
	{ "li",	STR,	0,	"link-init",	off(linkinit), },
	{ "so",	STR,	0,	"link-options",	off(linkoptions), },
	{ "lu",	STR,	0,	"link-up",	off(linkup), },
	{ "pn",	STR,	&PN,	"number",	},
	{ "pn",	STR,	&PN,	"phone-number",	},
	{ 0,    STR,	0,	"proxy",	off(proxy), },
	{ "sl",	BIT,	0,	"slip",		off(flags),	F_SLIP},
	{ "br",	NUM,	0,	"speed",	off(speed), },
	{ "ms",	STR,	0,	"stty-modes",	off(sttymodes), },

	{ "im",	BIT,	0,	"immediate",	off(flags),	F_IMMEDIATE },
	{ "in",	NUM,	0,	"interface",	off(interface), },
	{ 0,	BIT,	0,	"nolastlog",	off(flags),	F_NOLASTLOG },

#define	PD	P &debug
	{ 0,	BIT,	PD,	"debug-all",	0,		(u_long)-1, },
	{ 0,	BIT,	PD,	"debug-int",	0,		D_DEBUG, },
	{ 0,	BIT,	PD,	"debug-packet",	0,		D_PKT, },
	{ 0,	BIT,	PD,	"debug-phase",	0,		D_MAIN, },
	{ 0,	BIT,	PD,	"debug-state",	0,		D_STATE, },
	{ 0,	BIT,	PD,	"packet-dump",	0,		D_TRC, },
	{ 0,	BIT,	PD,	"trace",	0,		D_DT, },
#undef	PD

	{ 0,	0,	0 },
};

#undef	off
#define	off(x)	(((char *)&((ppp_t *)0)->x) - (char *)0)

static struct captab ppptab[] = {
	{ "cm",	CMAP,	0,	"cmap",		off(ppp_rcmap), },
	{ 0,	CMAP,	0,	"tx-cmap",	off(ppp_tcmap), },
	{ 0,	CMAP,	0,	"rx-cmap",	off(ppp_rcmap), },
	{ 0,	TIME,	0,	"echo-freq",	off(ppp_echofreq), },
	{ "id",	TIME,	0,	"idle-timeout", off(ppp_idletime), },
	{ "mr",	NUM,	0,	"mru",		off(ppp_mru), },
	{ "mc",	NUM,	0,	"retries",	off(ppp_maxconf), },
	{ 0,	TIME,	0,	"retry-timeout",off(ppp_timeout), },
	{ "mt",	NUM,	0,	"term-retries",	off(ppp_maxterm), },
	{ "wt",	TIME,	0,	"timeout",	off(ppp_bostimeout), },
	{ "to",	NUM,	P &TO,	},


	{ 0,	NUM,	0,	"auth-retries",	off(ppp_authretries), },

	{ 0,	STR,	0,	"pap-peerid",	off(ppp_peerid), },
	{ 0,	STR,	0,	"pap-passwd",	off(ppp_passwd), },

	{ 0,	NUM,	0,	"chap-mode",	off(ppp_chapmode), },
	{ 0,	STR,	0,	"chap-script",	off(ppp_chapscript), },
	{ 0,	STR,	0,	"chap-name",	off(ppp_chapname), },
	{ 0,	STR,	0,	"chap-secret",	off(ppp_chapsecret), },

	{ 0,	ADDR,	0,	"local-addr",	off(ppp_us), },
	{ 0,	ADDR,	0,	"remote-addr",	off(ppp_them), },

	{ 0,	ADDR,	0,	"primary-dns",	off(ppp_dns[0]), },
	{ 0,	ADDR,	0,	"secondary-dns",off(ppp_dns[1]), },
	{ 0,	ADDR,	0,	"primary-nbs",	off(ppp_nbs[0]), },
	{ 0,	ADDR,	0,	"secondary-nbs",off(ppp_nbs[1]), },

	{ "pf",	STR,	&PF,	},
	{ 0,	BIT,	0,	"acfc",		off(ppp_flags),	PPP_ACFC, },
	{ 0,	BIT,	0,	"chap",		off(ppp_flags),	PPP_USECHAP, },
	{ 0,	BIT,	0,	"chap-allow",	off(ppp_flags),	PPP_CANCHAP, },
	{ 0,	BIT,	0,	"ftel",		off(ppp_flags),	PPP_FTEL, },
	{ 0,	BIT,	0,	"pap",		off(ppp_flags),	PPP_USEPAP, },
	{ 0,	BIT,	0,	"pfc",		off(ppp_flags),	PPP_PFC, },
	{ 0,	BIT,	0,	"tcpc",		off(ppp_flags),	PPP_TCPC, },
	{ 0,	BIT,	0, "allow-addr-change",	off(ppp_flags),	PPP_CHGADDR,},

	{ 0,	BIT,	0,	"multilink",	off(ppp_flags),	PPP_USEML, },
	{ 0,	NUM,	0,	"mrru",		off(ppp_mrru), },
	{ 0,	BIT,	0,	"sseq",		off(ppp_flags), PPP_SSEQ, },
	{ 0,	ADDR_NULL, 0,	"ed-null", 	off(ppp_snd_ed), },
	{ 0,	ADDR_LOCAL, 0,	"ed-local", 	off(ppp_snd_ed), },
	{ 0,	ADDR_802, 0,	"ed-802.1", 	off(ppp_snd_ed), },
	{ 0,	ADDR_IP, 0,	"ed-ip", 	off(ppp_snd_ed), },
	{ 0,	CONST,	0,	"first-up",	off(ppp_pifflags),
						    PIF_FIRST_UP, ~PIF_MASK },
	{ 0,	CONST,	0,	"first-idle",	off(ppp_pifflags),
						    PIF_FIRST_IDLE, ~PIF_MASK },
	{ 0,	CONST,	0,	"next-up",	off(ppp_pifflags),
						    PIF_NEXT_UP, ~PIF_MASK },
	{ 0,	CONST,	0,	"next-idle",	off(ppp_pifflags),
						    PIF_NEXT_IDLE, ~PIF_MASK },

	{ 0,	BIT,	0,	"ipv4",		off(ppp_flags),	PPP_WANTV4, },
	{ 0,	BIT,	0,	"ipv6",		off(ppp_flags),	PPP_WANTV6, },

	{ 0,	0,	0 },
};

static struct captab pppauth[] = {
	{ "id",	TIME,	0,	"idle-timeout", off(ppp_idletime), },
	{ 0,	TIME,	0,	"echo-freq",	off(ppp_echofreq), },

	{ 0,	ADDR,	0,	"local-addr",	off(ppp_us), },
	{ 0,	ADDR,	0,	"remote-addr",	off(ppp_them), },

	{ 0,	ADDR,	0,	"primary-dns",	off(ppp_dns[0]), },
	{ 0,	ADDR,	0,	"secondary-dns",off(ppp_dns[1]), },
	{ 0,	ADDR,	0,	"primary-nbs",	off(ppp_nbs[0]), },
	{ 0,	ADDR,	0,	"secondary-nbs",off(ppp_nbs[1]), },

#define	PD	P &debug
	{ 0,	BIT,	PD,	"trace",	0,		D_DT, },
	{ 0,	BIT,	PD,	"packet-dump",	0,		D_TRC, },
	{ 0,	BIT,	PD,	"debug-packet",	0,		D_PKT, },
	{ 0,	BIT,	PD,	"debug-state",	0,		D_STATE, },
	{ 0,	BIT,	PD,	"debug-int",	0,		D_DEBUG, },
	{ 0,	BIT,	PD,	"debug-phase",	0,		D_MAIN, },
	{ 0,	BIT,	PD,	"debug-all",	0,		(u_long)-1, },
#undef	PD

	{ "lf",	STR,	&LF,	"link-failed",	},
	{ "li",	STR,	&LI,	"link-init",	},
	{ "so", STR,	&SO,	"link-options",	},
	{ "ld",	STR,	&LD,	"link-down",	},
	{ "lu",	STR,	&LU,	"link-up",	},

	{ 0,	BIT,	0,	"ftel",	off(ppp_flags),	PPP_FTEL, },
	{ 0,	BIT,	0,	"tcpc",	off(ppp_flags),	PPP_TCPC, },
	{ 0,	BIT,	0, "allow-addr-change",	off(ppp_flags),	PPP_CHGADDR, },

	{ 0,	BIT,	0,	"multilink",	off(ppp_flags),	PPP_USEML, },
	{ 0,	NUM,	0,	"mrru",		off(ppp_mrru), },
	{ 0,	BIT,	0,	"sseq",		off(ppp_flags), PPP_SSEQ, },
	{ 0,	ADDR_NULL, 0,	"ed-null", 	off(ppp_snd_ed), },
	{ 0,	ADDR_LOCAL, 0,	"ed-local", 	off(ppp_snd_ed), },
	{ 0,	ADDR_802, 0,	"ed-802.1", 	off(ppp_snd_ed), },
	{ 0,	ADDR_IP, 0,	"ed-ip", 	off(ppp_snd_ed), },
	{ 0,	CONST,	0,	"first-up",	off(ppp_pifflags),
						    PIF_FIRST_UP, ~PIF_MASK },
	{ 0,	CONST,	0,	"first-idle",	off(ppp_pifflags),
						    PIF_FIRST_IDLE, ~PIF_MASK },
	{ 0,	CONST,	0,	"next-up",	off(ppp_pifflags),
						    PIF_NEXT_UP, ~PIF_MASK },
	{ 0,	CONST,	0,	"next-idle",	off(ppp_pifflags),
						    PIF_NEXT_IDLE, ~PIF_MASK },

	{ 0,	0,	0 },
};

static struct captab sltab[] = {
	{ 0,	0,	0 },
};

static int gstr();
static int gint();
static int gbool();
static void getcap_internal();
int pppcmap();

session_t *
getsyscap(host, file)
	register char *host;
	char *file;
{
	int stat;
	char **av, *buf, *p;
	session_t *s;
	char *capfiles[] = { file ? file : _PATH_SYS, 0 };

	if (file && file[0] != '/' && access(file, R_OK) != 0) {
		capfiles[0] = _PATH_SYS;
		cgetset(file);
	}

	if ((s = malloc(sizeof(*s))) == NULL)
		err(1, NULL);
	memset(s, 0, sizeof(*s));
	s->sysname = host;
	s->fd = -1;
	s->oldld = -1;

	if ((stat = cgetent(&buf, capfiles, host)) != 0) {
		switch (stat) {
		case 1:
			warnx("couldn't resolve 'tc' for %s in %s",
				host, capfiles[0]);
			break;
		case -1:
			warnx("host '%s' unknown in %s", host, capfiles[0]);
			break;
		case -2:
			warn("%s: getting host data in %s", host, capfiles[0]);
			break;
		case -3:
			warnx("'tc' reference loop for %s in %s",
				host, capfiles[0]);
			break;
		default:
			warnx("ppp: unexpected cgetent() error for %s in %s",
				host, capfiles[0]);
			break;
		}		
		exit(1);
	}

	s->buf = buf;
	getcap_internal(buf, captab, s);

	/* error if direct is set and no device specified */
	if ((s->flags & F_DIRECT) && s->device == NULL)
		errx(1, "no device specified for %s in %s", host, capfiles[0]);

	if (PN) {
		stat = 2;
		for (p = PN; *p; ++p)
			if (*p == '|')
				++stat;
		if ((av = malloc(sizeof(char *) * stat)) == NULL)
			err(1, NULL);

		s->destinations = av;
		
		p = *av++ = PN;
		while ((p = strchr(p, '|')) != NULL) {
			*p++ = '\0';
			*av++ = p;
		}
		*av++ = NULL;
		*av++ = NULL;
	} else {
		if ((s->destinations = malloc(sizeof(char *) * 2)) == NULL)
			err(1, NULL);
		s->destinations[0] = NULL;
		s->destinations[1] = NULL;
	}

	return(s);
}

int
getauthcap(ppp, host, file, errbuf, errlen)
	ppp_t *ppp;
	register char *host;
	char *file;
	char *errbuf;
	int errlen;
{
	session_t *S;
	int stat;
	char *buf;
	char *capfiles[] = { file ? file : _PATH_SYS, 0 };

	if (file && file[0] != '/' && access(file, R_OK) != 0) {
		capfiles[0] = _PATH_SYS;
		cgetset(file);
	}

	if ((stat = cgetent(&buf, capfiles, host)) != GETAUTHCAP_OK) {
		switch (stat) {
		case GETAUTHCAP_BADTC:
			snprintf(errbuf, errlen,
			    "%s: couldn't resolve tc", host);
			break;
		case GETAUTHCAP_NOHOST:
			snprintf(errbuf, errlen,
			    "%s: unknown host", host);
			break;
		case GETAUTHCAP_NODATA:
			snprintf(errbuf, errlen,
			    "%s: missing host data");
			break;
		case GETAUTHCAP_TCLOOP:
			snprintf(errbuf, errlen,
			    "%s: 'tc' reference loop for", host);
			break;
		default:
			snprintf(errbuf, errlen,
			    "%s: unexpected cgetent() error", host);
			break;
		}		
		return(stat);
	}

	LF = LI = LD = LU = SO = 0;
	getcap_internal(buf, pppauth, ppp);
	if ((S = (session_t *)ppp->ppp_session) != NULL) {
		if (LD)
			S->linkdown = LD;
		if (LF)
			S->linkfailed = LF;
		if (LI)
			S->linkinit = LI;
		if (LU)
			S->linkup = LU;
		if (SO)
			S->linkoptions = SO;
	}

	return(0);
}

void
getslcap(session_t *s)
{
	getcap_internal(s->buf, sltab, s->private);
	s->flags |= F_IMMEDIATE;	/* we force immediate mode */
	s->interface = -1;		/* we can never assign the interface */
	/*
	 * If we find the "s0" or "e0" sequences and we don't already
	 * have a proxy specified, default to /usr/libexec/ppp_proxy
	 */
	if (s->proxy == NULL) {
		int i;
		char *res;
		char S[3], E[3];
		S[0] = 's'; S[2] = 0;
		E[0] = 'e'; E[2] = 0;
		for (i = '0'; i <= '9'; ++i) {
			S[1] = i;
			E[1] = i;
			if ((cgetstr(s->buf, S, &res) >= 0 && *res) ||
			    (cgetstr(s->buf, E, &res) >= 0 && *res)) {
				s->proxy = _PATH_PPPPROXY;
				break;
			}
		}
	}
}

void
getpppcap(session_t *s)
{
	ppp_t *ppp = (ppp_t *)s->private;

	if (ppp->ppp_flags & PPP_ASYNC)
		ppp->ppp_flags |= PPP_PFC | PPP_ACFC | PPP_TCPC;

	ppp->ppp_flags |= PPP_FTEL;

	ppp->ppp_snd_ed.ed_type = -1;		/* no ed-* specified */
	ppp->ppp_pifflags = PIF_NEXT_IDLE;

	getcap_internal(s->buf, ppptab, (char *)ppp);

	/*
	 * If we find the "s0" or "e0" sequences and we don't already
	 * have a proxy specified, default to /usr/libexec/ppp_proxy
	 */
	if (s->proxy == NULL) {
		int i;
		char *res;
		char S[3], E[3];
		S[0] = 's'; S[2] = 0;
		E[0] = 'e'; E[2] = 0;
		for (i = '0'; i <= '9'; ++i) {
			S[1] = i;
			E[1] = i;
			if ((cgetstr(s->buf, S, &res) >= 0 && *res) ||
			    (cgetstr(s->buf, E, &res) >= 0 && *res)) {
				s->proxy = _PATH_PPPPROXY;
				break;
			}
		}
	}

	if (ppp->ppp_timeout.tv_sec == 0 && ppp->ppp_timeout.tv_usec == 0 &&
	    TO > 0) {
		ppp->ppp_timeout.tv_sec = TO / 10;
		ppp->ppp_timeout.tv_usec = (TO % 10) * 100000;
	}
	if (ppp->ppp_timeout.tv_sec == 0 && ppp->ppp_timeout.tv_usec == 0)
		ppp->ppp_timeout.tv_sec = 3;

	if (ppp->ppp_bostimeout.tv_sec == 0 && ppp->ppp_bostimeout.tv_usec == 0)
		ppp->ppp_bostimeout.tv_sec = 60;

	if (ppp->ppp_mru <= 0)
		ppp->ppp_mru = LCP_DFLT_MRU;

	if (ppp->ppp_maxterm <= 0)
		ppp->ppp_maxterm = 3;

	if (ppp->ppp_maxconf <= 0)
		ppp->ppp_maxconf = 10;

	if (PF && *PF) {
		char *p;
		char sc;
		int neg;

		static struct {
			char    *n;
			int     v;
		} *t, tab[] = {
			{ "pfc",	PPP_PFC },
			{ "acfc",	PPP_ACFC },
			{ "tcpc",	PPP_TCPC },
			{ "ftel",	PPP_FTEL },
			{ "trace",	PPP_TRACE },
			{ 0,   		0 },
		};

		while (*PF) {
			while (*PF == ',' || *PF == ' ')
				PF++;
			if (*PF == 0)
				break;
			p = PF;
			while (*PF && *PF != ',' && *PF != ' ')
				PF++;
			sc = *PF;
			*PF = 0;

			neg = 0;
			if (*p == '-')
				p++, neg++;
			for (t = tab; t->n && strcmp(t->n, p); t++)
				;
			if (t->n == 0) {
				fprintf(stderr, "ppp: bad flag %s for %s\n",
					p, s->sysname);
				exit(1);
			}
			*PF = sc;
			if (neg)
				ppp->ppp_flags &= ~(t->v);
			else
				ppp->ppp_flags |= t->v;
		}
	}

	if (ppp->ppp_flags & PPP_TRACE)
		debug |= D_DT;

	if (ppp->ppp_peerid && ppp->ppp_passwd)
		ppp->ppp_flags |= PPP_CANPAP;
	else if (ppp->ppp_peerid || ppp->ppp_passwd)
		err(1, "Only one of pap_peerid and pap_passwd were specified");

	/*
	 * Dang thing defaults to -1!
	 */
	if (ppp->ppp_chapmode < 0)
		ppp->ppp_chapmode = 0;

	if (ppp->ppp_chapmode == 0)
		ppp->ppp_chapmode = CHAP_MD5;

	if (ppp->ppp_chapscript == 0)
		ppp->ppp_chapscript = _PATH_CHAPSCRIPT;

	if (ppp->ppp_chapname == 0) {
		static char hostname[MAXHOSTNAMELEN];

		if (gethostname(hostname, sizeof(hostname)) < 0)
			err(1, "trying to get local host name");
		ppp->ppp_chapname = hostname;
	}

	if (strlen(ppp->ppp_chapname) >= MAXHOSTNAMELEN)
		errx(1, "%s: name too long", ppp->ppp_chapname);

	if ((ppp->ppp_us || ppp->ppp_them) && s->linkinit)
		err(1, "Cannot specify both link-init (li) script and local-addr/remote-addr.\n");
}

int
setdebug(char *level)
{
      struct captab *t;

      for (t = captab; t->name || t->oldname; t++) {
	      if (t->name && t->ptr == P &debug) {
		      if (strcmp(level, t->name) == 0) {
			      debug |= t->bit;
			      return (0);
		      }
	      }
      }
      return (-1);
}


static int
getladdr(str, buf, buflen)
	char *str, *buf;
	int buflen;
{
	int i;
	char *p;
	unsigned long t;

	for (i = 0; i < buflen; i++) {
		p = strsep(&str, "-.");
		if (p == NULL)
			return(i);
		t = strtoul(p, NULL, 16);
		if (t > 255)
			return(0);
		buf[i] = t;
	}
	return(0);
}

static void
getcap_internal(buf, t, base)
	char *buf;
	struct captab *t;
	char *base;
{
	double d;
	int stat;
	char *res;
	struct timeval *tv;
	struct hostent *he;
	struct in_addr ia;
	struct ppp_ed *ed;
	char **loc;

	for (; t->name || t->oldname; t++) {
		if (t->ptr)
			loc = t->ptr;
		else
			loc = (char **)(base + t->offset);
		res = 0;

		switch (t->type) {
		case STR:
			if (gstr(buf, t, &res))
				*loc = res;
			break;

		case NUM:
			if (gstr(buf, t, &res))
				*(int *)loc = strtol(res, 0, 0);
			else
				*(int *)loc = gint(buf, t);
			break;

		case CMAP:
			if (gstr(buf, t, &res) && pppcmap(res, loc) < 0)
				err(1, "bad character map");
			break;

		case TIME:
			if (gstr(buf, t, &res))
				d = strtod(res, 0);
			else
				d = gint(buf, t);
			tv = (struct timeval *)loc;
			if (d >= 0.0) {
				tv->tv_sec = d;
				d -= tv->tv_sec;
				tv->tv_usec = d * 1000000.0;
			} else {
				tv->tv_sec = 0;
				tv->tv_usec = 0;
			}
			break;

		case ADDR_NULL:
			if (gbool(buf, t) == 1) {
				ed = (struct ppp_ed *)loc;
				memset(ed, 0, sizeof(*ed));
				ed->ed_type = ED_NULL;
				ed->ed_len = 0;
			}
			break;

		case ADDR_LOCAL:
			/*
			 * The local address format that we generate
			 * is 4 bytes based on the boot time, followed
			 * by up to 16 bytes of the hostname.
			 */
			if (gbool(buf, t) == 1) {
				int mib[] = { CTL_KERN, KERN_BOOTTIME };
				struct timeval *tvp;
				size_t tvplen;

				ed = (struct ppp_ed *)loc;
				memset(ed, 0, sizeof(*ed));
				tvp = (struct timeval *)ed->ed;
				ed->ed_type = ED_LOCAL;
				ed->ed_len = sizeof(ed->ed) < 20 ?
				    sizeof(ed->ed) : 20;
				tvplen = sizeof(*tvp);
				sysctl(mib, 2, tvp, &tvplen, NULL, 0);
				tvp->tv_sec += tvp->tv_usec;
				gethostname((char *)&ed->ed[sizeof(long)],
					ed->ed_len - sizeof(long));
			}
			break;

		case ADDR_802:
			if (gstr(buf, t, &res)) {
				ed = (struct ppp_ed *)loc;
				memset(ed, 0, sizeof(*ed));
				if (getladdr(res, ed->ed, sizeof(ed->ed)) != 6)
					errx(1, "%s: bad 802.1 address\n", res);
				ed->ed_type = ED_802_1;
				ed->ed_len = 6;
			}
			break;

		case ADDR_IP:
			if (gstr(buf, t, &res)) {
				ed = (struct ppp_ed *)loc;
				memset(ed, 0, sizeof(*ed));
				ed->ed_type = ED_IP;
				ed->ed_len = 4;
				loc = (char **)ed->ed;
				goto do_ip_addr;
			}
			break;

		case ADDR:
			ia.s_addr = 0;

			if (gstr(buf, t, &res)) {
			do_ip_addr:
				if ((he = gethostbyname(res)) == NULL) {
					herror(res);
					exit(1);
				}
				if (he->h_addrtype == AF_INET &&
				    he->h_length == INT32SZ)
					memcpy(&ia, he->h_addr, INT32SZ);
				else
					errx(1, "%s: invalid address type",res);
			}
			*(u_long *)loc = ia.s_addr;
			break;

		case CONST:
			stat = gbool(buf, t);
			if (stat < 0)
				break;
			*(u_long *)loc &= t->mask;
			*(u_long *)loc |= t->bit;
			break;
				
		case BIT:
			stat = gbool(buf, t);
			if (stat < 0)
				break;
			if (stat)
				*(u_long *)loc |= t->bit;
			else
				*(u_long *)loc &= ~t->bit;
			break;
		}
	}
}

static int
gstr(buf, t, res)
	char *buf;
	struct captab *t;
	char **res;
{
	if (t->name && cgetstr(buf, t->name, res) >= 0 && *res && **res)
		return(1);
	if (t->oldname && cgetstr(buf, t->oldname, res) >= 0)
		return(1);
	return(0);
}

static int
gint(buf, t)
	char *buf;
	struct captab *t;
{
	long res;

	if (t->name && cgetnum(buf, t->name, &res) >= 0)
		return((int)res);
	if (t->oldname && cgetnum(buf, t->oldname, &res) >= 0)
		return((int)res);
	return(-1);
}

static int
gbool(buf, t)
	char *buf;
	struct captab *t;
{
	char nb[64];

	if (t->name && cgetcap(buf, t->name, ':') != NULL)
		return(1);
	if (t->oldname && cgetcap(buf, t->oldname, ':') != NULL)
		return(1);
	if (t->name) {
		snprintf(nb, sizeof(nb), "-%s", t->name);
		if (cgetcap(buf, nb, ':') != NULL)
			return(0);
	}
	if (t->oldname) {
		snprintf(nb, sizeof(nb), "-%s", t->name);
		if (cgetcap(buf, nb, ':') != NULL)
			return(0);
	}
	return(-1);
}

/*
 * Parse PPP control characters map
 */
int
pppcmap(arg, res)
	char *arg;
	u_long *res;
{
	u_long map = 0;
	char c;

	if (*arg == '0') {
		/* Hexadecimal? */
		if (*++arg == 'x' || *arg == 'X') {
			while ((c = *++arg) != NULL) {
				map <<= 4;
				if ('0' <= c && c <= '9')
					map |= c - '0';
				else if ('A' <= c && c <= 'F')
					map |= c - 'A' + 10;
				else if ('a' <= c && c <= 'f')
					map |= c - 'a' + 10;
				else
					return (-1);
			}
		} else {
			/* Octal? */
			while ((c = *arg++) != NULL) {
				map <<= 3;
				if ('0' <= c && c <= '7')
					map |= c - '0';
				else
					return (-1);
			}
		}
	} else {
		/* List of characters? */
		while ((c = *arg++) != NULL)
			map |= 1 << (c & 037);
	}
	*res = map;
	return (0);
}
