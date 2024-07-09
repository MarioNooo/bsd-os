/*	BSDI mkmakefile.c,v 2.15 2002/04/24 17:34:42 prb Exp	*/

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
 *	@(#)mkmakefile.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "template.h"

/*
 * Make the Makefile.
 */

static const char *srcpath __P((struct files *));

static int emitdefs __P((FILE *));
static int emitfiles __P((FILE *, int));

static int emitcfiles __P((FILE *));
static int emitload __P((FILE *));
static int emitobjs __P((FILE *));
static int emitrules __P((FILE *));
static int emitsfiles __P((FILE *));

const char *objpath;
static int have_source;

static struct template templates[] = {
	{ "CFILES", 0, 0, emitcfiles },
	{ "LOAD", 0, 0, emitload },
	{ "OBJS", 0, 0, emitobjs },
	{ "RULES", 0, 0, emitrules },
	{ "SFILES", 0, 0, emitsfiles },
};

/*
 * A file is considered to be in a BSP directory if it starts with
 * either ./ or has no / in the path name.
 */
#define	is_bsp(file)	(!oldformat && (strncmp(file, "./", 2) == 0 || \
					     strchr(file, '/') == NULL))

int
mkmakefile()
{

	have_source = ht_lookup(selecttab, intern("source")) != NULL;
	return (template("Makefile", emitdefs, T_SIMPLE,
	    templates, sizeof templates / sizeof *templates));
}

void
setobjpath(op)
	const char *op;
{

	if (oldformat && objpath != NULL)
		error("object path `%s' already set", objpath);
	objpath = intern(op);
}

/*
 * Return (possibly in a static buffer) the name of the `source' for a
 * file.  If we have `options source', or if the file is marked `always
 * source', this is always the path from the `file' line; otherwise we
 * get the .o from the obj-directory.
 */
static const char *
srcpath(fi)
	register struct files *fi;
{
	static char buf[MAXPATHLEN];

	if (have_source || (fi->fi_flags & FI_ALWAYSSRC) != 0) {
		if (fi->fi_root == NULL) {
			return (fi->fi_path);
		}
		(void)snprintf(buf, sizeof buf, "%s/%s", fi->fi_root, fi->fi_path);
		return (buf);
	}
	if (fi->fi_obj == NULL) {
		if (!oldformat || objpath == NULL) {
			error("obj-directory not set");
			return (NULL);
		}
		fi->fi_obj = objpath;
	}
	(void)snprintf(buf, sizeof buf, "%s/%s.o", fi->fi_obj, fi->fi_base);
	return (buf);
}

static int
emitdefs(fp)
	register FILE *fp;
{
	register struct nvlist *nv;
	register char *sp;
	const char *source;

	if (check("SYS") == NULL)
		assign("SYS", "../..", 0);
	if (!oldformat && check("CONFDIR") == NULL)
		assign("CONFDIR", "../..", 0);
	if (emitvars(fp) != 0)
		return (1);
	if (fputs("IDENT=", fp) < 0)
		return (1);
	sp = "";
	for (nv = options; nv != NULL; nv = nv->nv_next) {
		if (fprintf(fp, "%s-D%s%s%s", sp, nv->nv_name,
		    nv->nv_str ? "=" : "", nv->nv_str ? nv->nv_str : "") < 0)
			return (1);
		sp = " ";
	}
	if (putc('\n', fp) < 0)
		return (1);
	if (fprintf(fp, "PARAM=-DMAXUSERS=%d", maxusers) < 0)
		return (1);
	if (ktimezone)
		if (fprintf(fp, " -DTIMEZONE=%d", ktimezone) < 0)
			return (1);
	if (putc('\n', fp) < 0)
		return (1);
	for (nv = mkoptions; nv != NULL; nv = nv->nv_next)
		if (fprintf(fp, "%s=%s\n", nv->nv_name, nv->nv_str) < 0)
			return (1);

	if (!oldformat && (source = check("SYS")) != NULL) {
		if (*source != '/' && *source != '$') {
			if (fprintf(fp, "S=${CONFDIR}/%s\n", source) < 0)
				return (1);
		} else {
			if (fprintf(fp, "S=%s\n", source) < 0)
				return (1);
		}
		if (fprintf(fp, "B=${CONFDIR}\n") < 0)
			return (1);
	}
	return (0);
}

static int
emitobjs(fp)
	register FILE *fp;
{
	register struct files *fi;
	register int lpos, len, sp;

	if (fputs("OBJS=", fp) < 0)
		return (1);
	sp = '\t';
	lpos = 7;
	for (fi = allfiles; fi != NULL; fi = fi->fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		len = strlen(fi->fi_base) + 2;
		if (lpos + len > 72) {
			if (fputs(" \\\n", fp) < 0)
				return (1);
			sp = '\t';
			lpos = 7;
		}
		if (fprintf(fp, "%c%s.o", sp, fi->fi_base) < 0)
			return (1);
		lpos += len + 1;
		sp = ' ';
	}
	if (lpos != 7 && putc('\n', fp) < 0)
		return (1);
	return (0);
}

static int
emitcfiles(fp)
	FILE *fp;
{

	return (emitfiles(fp, 'c'));
}

static int
emitsfiles(fp)
	FILE *fp;
{

	return (emitfiles(fp, 's'));
}

static int
emitfiles(fp, suffix)
	register FILE *fp;
	int suffix;
{
	register struct files *fi;
	register struct config *cf;
	register int lpos, len, sp;
	register const char *fpath;
	const char *prefix;
	char swapname[100];

	if (fprintf(fp, "%cFILES=", toupper(suffix)) < 0)
		return (1);
	sp = '\t';
	lpos = 7;
	for (fi = allfiles; fi != NULL; fi = fi->fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		if ((fpath = srcpath(fi)) == NULL)
			return (1);
		len = strlen(fpath);
		if (fpath[len - 1] != suffix)
			continue;
		if (*fpath != '/' && *fpath != '$') {
			if (is_bsp(fpath)) {
				prefix = "$B/";
				len += 3;
				if (fpath[1] == '/') {
					fpath += 2;
					len -= 2;
				}
			} else {
				prefix = "$S/";
				len += 3;
			}
		} else
			prefix = "";
		if (lpos + len > 72) {
			if (fputs(" \\\n", fp) < 0)
				return (1);
			sp = '\t';
			lpos = 7;
		}
		if (fprintf(fp, "%c%s%s", sp, prefix, fpath) < 0)
			return (1);
		lpos += len + 1;
		sp = ' ';
	}
	/*
	 * The allfiles list does not include the configuration-specific
	 * C source files.  These files should be eliminated someday, but
	 * for now, we have to add them to ${CFILES} (and only ${CFILES}).
	 */
	if (suffix == 'c') {
		for (cf = allcf; cf != NULL; cf = cf->cf_next) {
			if (cf->cf_root == NULL)
				(void)sprintf(swapname,
				    "$S/%s/%s/swapgeneric.c",
				    require("ARCH"), require("ARCH"));
			else
				(void)sprintf(swapname, "swap%s.c",
				    cf->cf_name);
			len = strlen(swapname);
			if (lpos + len > 72) {
				if (fputs(" \\\n", fp) < 0)
					return (1);
				sp = '\t';
				lpos = 7;
			}
			if (fprintf(fp, "%c%s", sp, swapname) < 0)
				return (1);
			lpos += len + 1;
			sp = ' ';
		}
	}
	if (putc('\n', fp) < 0)
		return (1);
	return (0);
}

/*
 * Emit the make-rules.
 */
static int
emitrules(fp)
	register FILE *fp;
{
	register struct files *fi;
	register const char *cp, *fpath;
	const char *prefix;
	int ch;
	char buf[200];

	for (fi = allfiles; fi != NULL; fi = fi->fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		if ((fpath = srcpath(fi)) == NULL)
			return (1);

		if (*fpath != '/' && *fpath != '$') {
			if (is_bsp(fpath)) {
				prefix = "$B/";
				if (fpath[1] == '/')
					fpath += 2;
			} else
				prefix = "$S/";
		} else
			prefix = "";

		if (fprintf(fp, "%s.o: %s%s\n", fi->fi_base, prefix, fpath) < 0)
			return (1);
		if ((have_source == 0 && !(fi->fi_flags & FI_ALWAYSSRC)) ||
		    (cp = fi->fi_mkrule) == NULL) {
			cp = fi->fi_flags & FI_DRIVER ? "DRIVER" : "NORMAL";
			ch = toupper((u_char)fpath[strlen(fpath) - 1]);
			(void)sprintf(buf, "${%s_%c%s}", cp, ch,
			    fi->fi_flags & FI_CONFIGDEP ? "_C" : "");
			cp = buf;
		}
		if (fprintf(fp, "\t%s\n\n", cp) < 0)
			return (1);
	}
	return (0);
}

/*
 * Emit the load commands.
 *
 * This function is not to be called `spurt'.
 */
static int
emitload(fp)
	register FILE *fp;
{
	register struct config *cf;
	register const char *nm, *swname;

	if (fputs("all:", fp) < 0)
		return (1);
	for (cf = allcf; cf != NULL; cf = cf->cf_next) {
		if (fprintf(fp, " %s", cf->cf_name) < 0)
			return (1);
	}
	if (fputs("\n\n", fp) < 0)
		return (1);
	for (cf = allcf; cf != NULL; cf = cf->cf_next) {
		nm = cf->cf_name;
		swname = cf->cf_root != NULL ? cf->cf_name : "generic";
		if (fprintf(fp, "%s: ${SYSTEM_DEP} swap%s.o", nm, swname) < 0)
			return (1);
		if (fprintf(fp, "\n\
\t${SYSTEM_LD_HEAD}\n\
\t${SYSTEM_LD} swap%s.o\n\
\t${SYSTEM_LD_TAIL}\n\
\n\
swap%s.o: ", swname, swname) < 0)
			return (1);
		if (cf->cf_root != NULL) {
			if (fprintf(fp, "swap%s.c\n", nm) < 0)
				return (1);
		} else {
			if (fprintf(fp, "${SYS}/%s/%s/swapgeneric.c\n",
			    require("ARCH"), require("ARCH")) < 0)
				return (1);
		}
		if (fputs("\t${NORMAL_C}\n\n", fp) < 0)
			return (1);
	}
	return (0);
}
