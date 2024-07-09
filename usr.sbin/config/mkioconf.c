/*	BSDI mkioconf.c,v 2.3 2001/12/10 19:05:58 prb Exp	*/

/* 
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mkioconf.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "template.h"

/*
 * Make ioconf.c.
 */
static int cforder __P((const void *, const void *));

static int emithdr __P((FILE *));
static int emitcfdata __P((FILE *));
static int emitcddecls __P((FILE *));
static int emitloc __P((FILE *));
static int emitpseudo __P((FILE *));
static int emitpv __P((FILE *));
static int emitroots __P((FILE *));
static int emitvec __P((FILE *));

static char *vecname __P((char *, const char *, int));

static int doconfig __P((FILE *));
static int dodevsw __P((FILE *));
static int dodeclsw __P((FILE *));
static int doifsel __P((FILE *));

static struct template templates[] = {
	{ "CONFIG", 0, 0, doconfig },
	{ "DECLSW", 0, 0, dodeclsw },
	{ "DEVSW", 1, 2, dodevsw },
	{ "IFSEL", 2, 3, doifsel },
};

#define	SEP(pos, max)	(((u_int)(pos) % (max)) == 0 ? "\n\t" : " ")

/*
 * NEWLINE can only be used in the emitXXX functions.
 * In most cases it can be subsumed into an fprintf.
 */
#define	NEWLINE		if (putc('\n', fp) < 0) return (1)

int
mkioconf()
{

	qsort(packed, npacked, sizeof *packed, cforder);
	return (template("ioconf.c", emithdr, 0,
	    templates, sizeof templates / sizeof *templates));
}

static int
cforder(a, b)
	const void *a, *b;
{
	register int n1, n2;

	n1 = (*(struct devi **)a)->i_cfindex;
	n2 = (*(struct devi **)b)->i_cfindex;
	return (n1 - n2);
}

/* XXX the SETLINE macro `knows' the inverse of the compile dir path */
#define	DO_LINE_DIRECTIVES
#ifdef DO_LINE_DIRECTIVES
#define SETLINE(fp, l, f) { \
	if (fprintf(fp, "#line %d \"../../%s/conf/%s\"\n", l, require("ARCH"), f) < 0) \
		return (1); \
}
#else
#define	SETLINE(fp, l, f)	/* nothing */
#endif

static int
emithdr(fp)
	register FILE *fp;
{

	if (fprintf(fp, "\
/*\n\
 * MACHINE GENERATED: DO NOT EDIT\n\
 *\n\
 * ioconf.c, from \"%s\"\n\
 */\n\n", conffile) < 0 )
		return (1);
	SETLINE(fp, 1, templateinfo.ti_ifname);
	return (0);
}

/*
 * True if any instance of a device or pseudo-device are configured.
 * Should use
 *	((dev)->d_ispseudo ? (dev)->d_umax != 0 : (dev)->d_ihead != NULL)
 * but the simpler define below also works.
 */
#define	ISCONF(dev)	((dev)->d_umax != 0 || (dev)->d_ihead != NULL)

/*
 * With one arg, %DEVSW(xx) means `emit "&xxsw" if any device xx is
 * configured in'.  With two args, %DEVSW(xx,yy) means `emit yy if
 * any device xx is configured in.'  In either case, we emit "NULL"
 * if the device is not configured.
 */
static int
dodevsw(fp)
	register FILE *fp;
{
	register struct devbase *dev;
	register const char *name, *str;
	char buf[100];

	name = templateinfo.ti_argv[0];
	dev = ht_lookup(devbasetab, name);
	if (dev == NULL) {
		xerror(templateinfo.ti_ifname, templateinfo.ti_lineno,
		    "`%s' is not a device", name);
		return (0);
	}
	if (ISCONF(dev)) {
		if ((str = templateinfo.ti_argv[1]) == NULL) {
			(void)sprintf(buf, "&%ssw", name);
			str = buf;
		}
	} else
		str = "NULL";
	return (fprintf(fp, "%s", str) < 0);
}

/*
 * With two arguments, %IFSEL(x,y) means `output y if x selected, else
 * output "NULL"'.  With three arguments, %IFSEL(x,y,z) means `output y
 * if x selected, else output z').
 */
static int
doifsel(fp)
	register FILE *fp;
{
	register const char *name, *str;

	name = templateinfo.ti_argv[0];
	if (ht_lookup(selecttab, name))
		str = templateinfo.ti_argv[1];
	else {
		if ((str = templateinfo.ti_argv[2]) == NULL)
			str = "NULL";
	}
	return (fprintf(fp, "%s", str) < 0);
}

/*
 * Emit devsw lines for all configured devices (including pseudo-devices).
 */
static int
dodeclsw(fp)
	register FILE *fp;
{
	register struct devbase *d;

	for (d = allbases; d != NULL; d = d->d_next) {
		if (ISCONF(d) &&
		    fprintf(fp, "extern struct devsw %ssw;\n", d->d_name) < 0)
			return (1);
	}
	SETLINE(fp, templateinfo.ti_lineno, templateinfo.ti_ifname);
	return (0);
}

/*
 * Emit the configuration data.
 */
static int
doconfig(fp)
	register FILE *fp;
{

	if (emitvec(fp) || emitcddecls(fp) || emitloc(fp) ||
	    emitpv(fp) || emitcfdata(fp) || emitroots(fp) ||
	    emitpseudo(fp))
		return (1);
	SETLINE(fp, templateinfo.ti_lineno, templateinfo.ti_ifname);
	return (0);
}

static int
emitcddecls(fp)
	register FILE *fp;
{
	register struct devbase *d;

	for (d = allbases; d != NULL; d = d->d_next) {
		if (d->d_ihead == NULL)
			continue;
		if (fprintf(fp, "extern struct cfdriver %scd;\n",
			    d->d_name) < 0)
			return (1);
	}
	return (putc('\n', fp) < 0);
}

static int
emitloc(fp)
	register FILE *fp;
{
	register int i;

	if (fprintf(fp, "\n/* locators */\n\
static int loc[%d] = {", locators.used) < 0)
		return (1);
	for (i = 0; i < locators.used; i++)
		if (fprintf(fp, "%s%s,", SEP(i, 8), locators.vec[i]) < 0)
			return (1);
	return (fprintf(fp, "\n};\n") < 0);
}

/*
 * Emit global parents-vector.
 */
static int
emitpv(fp)
	register FILE *fp;
{
	register int i;

	if (fprintf(fp, "\n/* parent vectors */\n\
static short pv[%d] = {", parents.used) < 0)
		return (1);
	for (i = 0; i < parents.used; i++)
		if (fprintf(fp, "%s%d,", SEP(i, 16), parents.vec[i]) < 0)
			return (1);
	return (fprintf(fp, "\n};\n") < 0);
}

/*
 * Emit the cfdata array.
 */
static int
emitcfdata(fp)
	register FILE *fp;
{
	register struct devi **p, *i, **par;
	register int unit, v;
	register const char *vs, *state, *basename;
	register struct nvlist *nv;
	register struct attr *a;
	char *loc;
	char locbuf[20];

	if (fprintf(fp, "\n\
#define NORM FSTATE_NOTFOUND\n\
#define STAR FSTATE_STAR\n\
\n\
struct cfdata cfdata[] = {\n\
\t/* driver     unit state    loc     flags parents ivstubs */\n") < 0)
		return (1);
	for (p = packed; (i = *p) != NULL; p++) {
		/* the description */
		if (fprintf(fp, "/*%3d: %s at ", i->i_cfindex, i->i_name) < 0)
			return (1);
		par = i->i_parents;
		for (v = 0; v < i->i_pvlen; v++)
			if (fprintf(fp, "%s%s", v == 0 ? "" : "|",
			    i->i_parents[v]->i_name) < 0)
				return (1);
		if (v == 0 && fputs("root", fp) < 0)
			return (1);
		a = i->i_atattr;
		nv = a->a_locs;
		for (nv = a->a_locs, v = 0; nv != NULL; nv = nv->nv_next, v++)
			if (fprintf(fp, " %s %s",
			    nv->nv_name, i->i_locs[v]) < 0)
				return (1);
		if (fputs(" */\n", fp) < 0)
			return (-1);

		/* then the actual defining line */
		basename = i->i_base->d_name;
		if (i->i_unit == STAR) {
			unit = i->i_base->d_umax;
			state = "STAR";
		} else {
			unit = i->i_unit;
			state = "NORM";
		}
		if (i->i_ivoff < 0) {
			vs = "";
			v = 0;
		} else {
			vs = "vec+";
			v = i->i_ivoff;
		}
		if (i->i_locoff >= 0) {
			(void)sprintf(locbuf, "loc+%3d", i->i_locoff);
			loc = locbuf;
		} else
			loc = "loc";
		if (fprintf(fp, "\
\t{&%scd,%s%2d, %s, %7s, %#6x, pv+%2d, %s%d},\n",
		    basename, strlen(basename) < 3 ? "\t\t" : "\t", unit,
		    state, loc, i->i_cfflags, i->i_pvoff, vs, v) < 0)
			return (1);
	}
	return (fputs("\t{0}\n};\n", fp) < 0);
}

/*
 * Emit the table of potential roots.
 */
static int
emitroots(fp)
	register FILE *fp;
{
	register struct devi **p, *i;

	if (fputs("\nshort cfroots[] = {\n", fp) < 0)
		return (1);
	for (p = packed; (i = *p) != NULL; p++) {
		if (i->i_at != NULL)
			continue;
		if (i->i_unit != 0 &&
		    (i->i_unit != STAR || i->i_base->d_umax != 0))
			(void)fprintf(stderr,
			    "config: warning: `%s at root' is not unit 0\n",
			    i->i_name);
		if (fprintf(fp, "\t%2d /* %s */,\n",
		    i->i_cfindex, i->i_name) < 0)
			return (1);
	}
	return (fputs("\t-1\n};\n", fp) < 0);
}

/*
 * Emit pseudo-device initialization.
 */
static int
emitpseudo(fp)
	register FILE *fp;
{
	register struct devi *i;
	register struct devbase *d;

	if (fputs("\n/* pseudo-devices */\n", fp) < 0)
		return (1);
	for (i = allpseudo; i != NULL; i = i->i_next)
		if (fprintf(fp, "extern void %sattach __P((int));\n",
		    i->i_base->d_name) < 0)
			return (1);
	if (fputs("\nstruct pdevinit pdevinit[] = {\n", fp) < 0)
		return (1);
	for (i = allpseudo; i != NULL; i = i->i_next) {
		d = i->i_base;
		if (fprintf(fp, "\t{ %sattach, %d },\n",
		    d->d_name, d->d_umax) < 0)
			return (1);
	}
	return (fputs("\t{ 0, 0 }\n};\n", fp) < 0);
}

/*
 * Emit interrupt vector declarations, and calculate offsets.
 */
static int
emitvec(fp)
	register FILE *fp;
{
	register struct nvlist *head, *nv;
	register struct devi **p, *i;
	register int j, nvec, unit;
	char buf[200];

	nvec = 0;
	for (p = packed; (i = *p) != NULL; p++) {
		if ((head = i->i_base->d_vectors) == NULL)
			continue;
		if ((unit = i->i_unit) == STAR)
			panic("emitvec unit==STAR");
		if (nvec == 0 && putc('\n', fp) < 0)
			return (1);
		for (j = 0, nv = head; nv != NULL; j++, nv = nv->nv_next)
			if (fprintf(fp,
			    "/* IVEC %s %d */ extern void %s();\n",
			    nv->nv_name, unit,
			    vecname(buf, nv->nv_name, unit)) < 0)
				return (1);
		nvec += j + 1;
	}
	if (nvec == 0)
		return (0);
	if (fprintf(fp, "\nstatic void (*vec[%d]) __P((void)) = {", nvec) < 0)
		return (1);
	nvec = 0;
	for (p = packed; (i = *p) != NULL; p++) {
		if ((head = i->i_base->d_vectors) == NULL)
			continue;
		i->i_ivoff = nvec;
		unit = i->i_unit;
		for (nv = head; nv != NULL; nv = nv->nv_next)
			if (fprintf(fp, "%s%s,",
			    SEP(nvec++, 4),
			    vecname(buf, nv->nv_name, unit)) < 0)
				return (1);
		if (fprintf(fp, "%s0,", SEP(nvec++, 4)) < 0)
			return (1);
	}
	return (fputs("\n};\n\n", fp) < 0);
}

static char *
vecname(buf, name, unit)
	char *buf;
	const char *name;
	int unit;
{

	(void)sprintf(buf, "X%s%d", name, unit);
	return (buf);
}
