/*	BSDI config.h,v 2.15 2003/04/01 16:05:50 prb Exp	*/

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
 *	@(#)config.h	8.1 (Berkeley) 6/6/93
 */

#ifdef        TORNADO
#include <sys/cdefs.h>
#include <sys/types.h>

#ifndef       __dead
#define       __dead  volatile
#endif        /* __dead */

#ifndef       NODEV
#define       NODEV   (dev_t) (-1)    /* non-existent device */
#endif        /* NODEV */
#endif        /* TORNADO */

extern int oldformat;
extern int importenv;

/*
 * Name/value lists.  Values can be strings or pointers and/or can carry
 * integers.  The names can be NULL, resulting in simple value lists.
 */
struct nvlist {
	struct	nvlist *nv_next;
	const char *nv_name;
	union {
		const char *u1_str;
		void *u1_ptr;
	} nv_u1;
#define	nv_str	nv_u1.u1_str
#define	nv_ptr	nv_u1.u1_ptr
	union {
		int	u2_int;
		dev_t	u2_dev;
	} nv_u2;
#define	nv_int	nv_u2.u2_int
#define	nv_dev	nv_u2.u2_dev
};

/*
 * Kernel configurations.
 */
struct config {
	struct	config *cf_next;	/* linked list */
	const char *cf_name;		/* "vmunix" */
	int	cf_lineno;		/* source line */
	struct	nvlist *cf_root;	/* "root on ra0a" */
	struct	nvlist *cf_swap;	/* "swap on ra0b and ra1b" */
	struct	nvlist *cf_dump;	/* "dumps on ra0b" */
};

/*
 * Attributes.  These come in two flavors: "plain" and "interface".
 * Plain attributes (e.g., "ether") simply serve to pull in files.
 * Interface attributes (e.g., "scsi") carry three lists: locators,
 * child devices, and references.  The locators are those things
 * that must be specified in order to configure a device instance
 * using this attribute (e.g., "tg0 at scsi0").  The a_devs field
 * lists child devices that can connect here (e.g., "tg"s), while
 * the a_refs are parents that carry the attribute (e.g., actual
 * SCSI host adapter drivers such as the SPARC "esp").
 */
struct attr {
	const char *a_name;		/* name of this attribute */
	int	a_iattr;		/* true => allows children */
	struct	nvlist *a_locs;		/* locators required */
	int	a_loclen;		/* length of above list */
	struct	nvlist *a_devs;		/* children */
	struct	nvlist *a_refs;		/* parents */
};

/*
 * The "base" part of a device ("uba", "sd"; but not "uba2" or
 * "sd0").  It may be found "at" one or more attributes, including
 * "at root" (this is represented by a NULL attribute).
 *
 * Each device may also export attributes.  If any provide an output
 * interface (e.g., "esp" provides "scsi"), other devices (e.g.,
 * "tg"s) can be found at instances of this one (e.g., "esp"s).
 * Such a connection must provide locators as specified by that
 * interface attribute (e.g., "target").
 *
 * Each base carries a list of instances (via d_ihead).  Note that this
 * list "skips over" aliases; those must be found through the instances
 * themselves.
 */
struct devbase {
	const char *d_name;		/* e.g., "sd" */
	struct	devbase *d_next;	/* linked list */
	int	d_isdef;		/* set once properly defined */
	int	d_ispseudo;		/* is a pseudo-device */
	int	d_major;		/* used for "root on sd0", e.g. */
	struct	nvlist *d_atlist;	/* e.g., "at tg" (attr list) */
	struct	nvlist *d_vectors;	/* interrupt vectors, if any */
	struct	nvlist *d_attrs;	/* attributes, if any */
	struct	devi *d_ihead;		/* first instance, if any */
	struct	devi **d_ipp;		/* used for tacking on more instances */
	int	d_umax;			/* highest unit number + 1 */
};

/*
 * An "instance" of a device.  The same instance may be listed more
 * than once, e.g., "xx0 at isa? port FOO" + "xx0 at isa? port BAR".
 *
 * After everything has been read in and verified, the devi's are
 * "packed" to collect all the information needed to generate ioconf.c.
 * In particular, we try to collapse multiple aliases into a single entry.
 * We then assign each "primary" (non-collapsed) instance a cfdata index.
 * Note that there may still be aliases among these.
 */
struct devi {
	/* created while parsing config file */
	const char *i_name;	/* e.g., "sd0" */
	int	i_unit;		/* unit from name, e.g., 0 */
	struct	devbase *i_base;/* e.g., pointer to "sd" base */
	struct	devi *i_next;	/* list of all instances */
	struct	devi *i_bsame;	/* list on same base */
	struct	devi *i_alias;	/* other aliases of this instance */
	const char *i_at;	/* where this is "at" (NULL if at root) */
	struct	attr *i_atattr;	/* attr that allowed attach */
	struct	devbase *i_atdev;/* dev if "at <devname><unit>", else NULL */
	const char **i_locs;	/* locators (as given by i_atattr) */
	int	i_atunit;	/* unit from "at" */
	int	i_cfflags;	/* flags from config line */
	int	i_lineno;	/* line # in config, for later errors */

	/* created during packing or ioconf.c generation */
/* 		i_loclen	   via i_atattr->a_loclen */
	short	i_collapsed;	/* set => this alias no longer needed */
	short	i_cfindex;	/* our index in cfdata */
	short	i_pvlen;	/* number of parents */
	short	i_pvoff;	/* offset in parents.vec */
	short	i_locoff;	/* offset in locators.vec */
	short	i_ivoff;	/* offset in interrupt vectors, if any */
	struct	devi **i_parents;/* the parents themselves */

};
/* special units */
#define	STAR	(-1)		/* unit number for, e.g., "sd*" */
#define	WILD	(-2)		/* unit number for, e.g., "sd?" */

/*
 * Files.  Each file is either standard (always included) or optional,
 * depending on whether it has names on which to *be* optional.  The
 * options field (fi_optx) is actually an expression tree, with nodes
 * for OR, AND, and NOT, as well as atoms (words) representing some
 * particular option.  The node type is stored in the nv_int field.
 * Subexpressions appear in the `next' field; for the binary operators
 * AND and OR, the left subexpression is first stored in the nv_ptr field.
 *
 * For any file marked as needs-count or needs-flag, fixfiles() will
 * build fi_optf, a `flat list' of the options with nv_int fields that
 * contain counts or `need' flags; this is used in mkheaders().
 */
struct files {
	struct	files *fi_next;	/* linked list */
	const char *fi_srcfile;	/* the name of the "files" file that got us */
	u_short	fi_srcline;	/* and the line number */
	u_char	fi_flags;	/* as below */
	const char *fi_path;	/* full file path */
	const char *fi_tail;	/* name, i.e., rindex(fi_path, '/') + 1 */
	const char *fi_base;	/* tail minus ".c" (or whatever) */
	const char *fi_root;	/* Where we are rooted */
	const char *fi_obj;	/* Where objects are rooted */
	struct	nvlist *fi_optx;/* options expression */
	struct	nvlist *fi_optf;/* flattened version of above, if needed */
	const char *fi_mkrule;	/* special make rule, if any */
};
#define	FX_ATOM		0	/* atom (in nv_name) */
#define	FX_NOT		1	/* NOT expr (subexpression in nv_next) */
#define	FX_AND		2	/* AND expr (lhs in nv_ptr, rhs in nv_next) */
#define	FX_OR		3	/* OR expr (lhs in nv_ptr, rhs in nv_next) */

/* flags */
#define	FI_SEL		0x01	/* selected */
#define	FI_CONFIGDEP	0x02	/* config-dependent */
#define	FI_DRIVER	0x04	/* device-driver */
#define	FI_NEEDSCOUNT	0x08	/* needs-count */
#define	FI_NEEDSFLAG	0x10	/* needs-flag */
#define	FI_HIDDEN	0x20	/* obscured by other(s), base names overlap */
#define	FI_ALWAYSSRC	0x40	/* otherwise use .o if not `options source' */

/*
 * Hash tables look up name=value pairs.  The pointer value of the name
 * is assumed to be constant forever; this can be arranged by interning
 * the name.  (This is fairly convenient since our lexer does this for
 * all identifier-like strings---it has to save them anyway, lest yacc's
 * look-ahead wipe out the current one.)
 */
struct hashtab;

const char *conffile;		/* source file, e.g., "GENERIC.sparc" */
const char *confdirbase;	/* basename of compile directory, usu. same */
int	errors;			/* counts calls to error() */
int	minmaxusers;		/* minimum "maxusers" parameter */
int	defmaxusers;		/* default "maxusers" parameter */
int	maxmaxusers;		/* default "maxusers" parameter */
int	maxusers;		/* configuration's "maxusers" parameter */
int	ktimezone;		/* configuration's timezone (i386 only) */
struct	nvlist *options;	/* options */
struct	nvlist *mkoptions;	/* makeoptions */
struct	hashtab *devbasetab;	/* devbase lookup */
struct	hashtab *selecttab;	/* selects things that are "optional foo" */
struct	hashtab *needcnttab;	/* retains names marked "needs-count" */

struct	devbase *allbases;	/* list of all devbase structures */
struct	config *allcf;		/* list of configured kernels */
struct	devi *alldevi;		/* list of all instances */
struct	devi *allpseudo;	/* list of all pseudo-devices */
int	ndevi;			/* number of devi's (before packing) */
int	npseudo;		/* number of pseudo's */

struct	files *allfiles;	/* list of all kernel source files */

struct	devi **packed;		/* arrayified table for packed devi's */
int	npacked;		/* size of packed table, <= ndevi */

struct {			/* pv[] table for config */
	short	*vec;
	int	used;
} parents;
struct {			/* loc[] table for config */
	const char **vec;
	int	used;
} locators;

/* files.c */
void	initfiles __P((void));
void	checkfiles __P((void));
int	fixfiles __P((void));	/* finalize */
void	setroot __P((const char *));
void	addfile __P((const char *, struct nvlist *, int, const char *, const char *));

/* hash.c */
struct	hashtab *ht_new __P((void));
int	ht_insrep __P((struct hashtab *, const char *, void *, int));
#define	ht_insert(ht, nam, val) ht_insrep(ht, nam, val, 0)
#define	ht_replace(ht, nam, val) ht_insrep(ht, nam, val, 1)
void	*ht_lookup __P((struct hashtab *, const char *));
void	initintern __P((void));
const char *intern __P((const char *));
const char *internn __P((const char *, int));
void	*hash_start __P((struct hashtab *));
const char *hash_next __P((void *, void **));
void	hash_end __P((void *));

/* main.c */
void	addoption __P((const char *name, const char *value));
void	addmkoption __P((const char *name, const char *value));

/* mkheaders.c */
int	mkheaders __P((void));

/* mkioconf.c */
int	mkioconf __P((void));

/* mkmakefile.c */
int	mkmakefile __P((void));
void	setobjpath __P((const char *));

/* mkswap.c */
int	mkswap __P((void));

/* mkswgeneric.c */
int	mkswgeneric __P((void));

/* pack.c */
void	pack __P((void));

/* scan.l */
int	currentline __P((void));
// int	yy_currentline __P((void));
int	cc_currentline __P((void));
int	mm_include __P((const char *, int));


/* sem.c, other than for yacc actions */
void	initsem __P((void));
int	mklocators __P((void));

/* util.c */
void	*emalloc __P((size_t));
void	*erealloc __P((void *, size_t));
char	*path __P((const char *));
char	*mkupper __P((const char *, char *));
void	error __P((const char *, ...));			/* immediate errs */
void	xerror __P((const char *, int, const char *, ...)); /* delayed errs */
__dead void panic __P((const char *, ...)) __attribute__((__volatile));
struct nvlist *newnvx __P((const char *,
	const char *, void *, int, struct nvlist *));
#define	newnv(n, s, p, i) newnvx(n, s, p, i, NULL)
void	nvfree __P((struct nvlist *));
void	nvfreel __P((struct nvlist *));
void	nvmaybefree __P((struct nvlist *));
#ifdef __CYGWIN__
const char	*convertposix __P((const char *name));
#else
#define convertposix(name)	intern(name)
#endif

/* variable.c */
const char	*check __P((const char *));
const char	*require __P((const char *));
const char	*assign __P((const char *, const char *, int));
char		*expand __P((const char *, int));
#ifdef	FOPEN_MAX
int		emitvars __P((FILE *));
#endif

/* The following hack allows config to compile under BSD/OS 1.1 */
#ifndef dk_unit
#undef major
#undef minor
#undef makedev
#define	major(x)	((int)(((u_int)(x) >> 20) & 0xfff))
#define	minor(x)	((int)((x) & 0xfffff))		/* minor number */
#define	makedev(x,y)	((dev_t)(((x) << 20) | (y)))	/* create dev_t */
#define	dk_unit(x)	((int)(((x) >> 10) & 0x3ff))	/* unit number */
#define	dk_part(x)	((int)((x) & 0x3ff))		/* subunit number */
#define	dk_makedev(x,y,z)	((dev_t)(((x)<<20) | ((y) << 10) | (z)))
#endif
