/*	BSDI pstat.c,v 2.14 2003/08/20 23:58:44 polk Exp	*/

/*-
 * Copyright (c) 1980, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1980, 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)pstat.c	8.17 (Berkeley) 9/18/95";
#endif /* not lint */

/*
 * We need the normally kernel-only struct tty definition (which requires
 * the normally kernel-only struct selinfo definition).
 */
#define _NEED_STRUCT_SELINFO
#define _NEED_STRUCT_TTY

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/ucred.h>
#define KERNEL
#include <sys/file.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#define NFS
#include <sys/mount.h>
#undef NFS
#include <sys/uio.h>
#include <sys/namei.h>
#include <miscfs/union/union.h>
#undef KERNEL
#include <sys/stat.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <vm/swap_pager.h>

#include <err.h>
#include <kvm.h>
#include <kvm_stat.h>
#include <limits.h>
#include <nlist.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

struct nlist nl[] = {
#define	FNL_NFILE	0
	{ "_nfiles" },
#define FNL_MAXFILE	1
	{ "_maxfiles" },

#ifdef __bsdi__

#define	VM_SWAPSTATS	2
	{"_swapstats"},
#define	VM_SWDEVLIST	3
	{"_swlist"},

#define NLMANDATORY	3	/* names up to here are mandatory */

#define	VM_KERNBASE	4
	{NL_KERNBASE},

#else

#define VM_SWAPMAP	2
	{ "_swapmap" },	/* list of free swap areas */
#define VM_NSWAPMAP	3
	{ "_nswapmap" },/* size of the swap map */
#define VM_SWDEVT	4
	{ "_swdevt" },	/* list of swap devices and sizes */
#define VM_NSWAP	5
	{ "_nswap" },	/* size of largest swap device */
#define VM_NSWDEV	6
	{ "_nswdev" },	/* number of swap devices */
#define VM_DMMAX	7
	{ "_dmmax" },	/* maximum size of a swap block */
#define NLMANDATORY	7	/* names up to here are mandatory */
#define VM_NISWAP	8
	{ "_niswap" },
#define VM_NISWDEV	9
	{ "_niswdev" },

#endif

	{ NULL }
};

int	usenumflag;
int	totalflag;
char	*nlistf	= NULL;
char	*memf = NULL;

struct {
	int m_flag;
	const char *m_name;
} mnt_flags[] = {
	{ MNT_ASYNC, "async" },
	{ MNT_DEFEXPORTED, "defexported" },
	{ MNT_DELEXPORT, "delete_export" },
	{ MNT_EXKERB, "exkerb" },
	{ MNT_EXPORTANON, "exportanon" },
	{ MNT_EXPORTED, "exported" },
	{ MNT_EXRDONLY, "exrdonly" },
	{ MNT_FORCE, "force" },
	{ MNT_LOCAL, "local" },
	{ MNT_MWAIT, "unmount_wait" },
	{ MNT_NODEV, "nodev" },
	{ MNT_NOEXEC, "noexec" },
	{ MNT_NOSUID, "nosuid" },
	{ MNT_QUOTA, "quota" },
	{ MNT_RDONLY, "rdonly" },
	{ MNT_RELOAD, "reload" },
	{ MNT_ROOTFS, "rootfs" },
	{ MNT_SYNCHRONOUS, "sync" },
	{ MNT_UNION, "union" },
	{ MNT_UNMOUNT, "unmount" },
	{ MNT_UPDATE, "update" },
	{ MNT_WAIT, "wait" },
	{ MNT_WANTRDWR, "wantrdwr" },
	{ 0 }
};


#define	SVAR(var) __STRING(var)	/* to force expansion */
#define	KGET(idx, var)							\
	KGET1(idx, &var, sizeof(var), SVAR(var))
#define	KGET1(idx, p, s, msg)						\
	KGET2(nl[idx].n_value, p, s, msg)
#define	KGET2(addr, p, s, msg)						\
	if (kvm_read(kd, (u_long)(addr), p, s) != s)			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd))
#define	KGETRET(addr, p, s, msg)					\
	if (kvm_read(kd, (u_long)(addr), p, s) != s) {			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd));	\
		return (0);						\
	}

void	filemode __P((void));
struct mount *
	getmnt __P((struct mount *));
void	mount_print __P((struct mount *));
void	nfs_header __P((void));
void	nfs_print __P((struct vnode *));
void	swapinfo __P((void));
int	td_compare __P((const void *, const void *));
void	ttymode __P((void));
void	ttyprt __P((struct ttystat *, int));
void	ufs_header __P((void));
void	ufs_print __P((struct vnode *));
void	union_header __P((void));
int	union_print __P((struct vnode *));
void	usage __P((void));
void	vnode_header __P((void));
void	vnode_print __P((struct vnode *, struct vnode *));
void	vnodemode __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	int ch, i, quit, ret;
	int fileflag, swapflag, ttyflag, vnodeflag;
	char buf[_POSIX2_LINE_MAX];

	fileflag = swapflag = ttyflag = vnodeflag = 0;
	while ((ch = getopt(argc, argv, "TM:N:finstv")) != EOF)
		switch (ch) {
		case 'f':
			fileflag = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			usenumflag = 1;
			break;
		case 's':
			swapflag = 1;
			break;
		case 'T':
			totalflag = 1;
			break;
		case 't':
			ttyflag = 1;
			break;
		case 'v':
		case 'i':		/* Backward compatibility. */
			vnodeflag = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL)
		(void)setgid(getgid());

	if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == 0)
		errx(1, "kvm_openfiles: %s", buf);
	if ((ret = kvm_nlist(kd, nl)) != 0) {
		if (ret == -1)
			errx(1, "kvm_nlist: %s", kvm_geterr(kd));
		for (i = quit = 0; i <= NLMANDATORY; i++)
			if (!nl[i].n_value) {
				quit = 1;
				warnx("undefined symbol: %s\n", nl[i].n_name);
			}
		if (quit)
			exit(1);
	}
	if (!(fileflag | vnodeflag | ttyflag | swapflag | totalflag))
		usage();
	if (fileflag || totalflag)
		filemode();
	if (vnodeflag || totalflag)
		vnodemode();
	if (ttyflag)
		ttymode();
	if (swapflag || totalflag)
		swapinfo();
	exit (0);
}

char *
fmtdev(dev, mode)
	dev_t dev;
	int mode;
{
	static char buf[30];
	char *name;

	if (usenumflag || (name = devname(dev, mode & S_IFMT)) == NULL)
		(void)snprintf(name = buf, sizeof(buf), "%d,%d,%d",
		    major(dev), dv_unit(dev), dv_subunit(dev));
	return (name);
}

char *
fmtflags(flags, fm, dash)
	int flags;
	struct flagmap *fm;
	int dash;
{
	static char buf[30];
	char *p;

	for (p = buf; flags != 0 && fm->fm_flag != 0; fm++) {
		if (flags & fm->fm_flag) {
			*p++ = fm->fm_char;
			flags &= ~fm->fm_flag;
		}
	}
	if (dash) {
		if (flags)				/* ??? */
			p += sprintf(p, "%#x", flags);	/* ??? */
		if (p == buf)
			*p++ = '-';
	}
	*p = 0;
	return (buf);
}

char *
fmttype(type, tm)
	int type;
	struct typemap *tm;
{

	while (tm->tm_type >= 0 && tm->tm_type != type)
		tm++;
	return (tm->tm_name);
}

void
vnodemode()
{
	struct e_vnode *e_vnodebase, *endvnode, *evp;
	struct vnode *vp;
	struct mount *maddr, *mp;
	int numvnodes;

	e_vnodebase = kvm_vnodes(kd, &numvnodes);
	if (e_vnodebase == NULL)
		errx(1, "kvm_vnodes: %s", kvm_geterr(kd));
	if (totalflag) {
		(void)printf("%7d vnodes\n", numvnodes);
		return;
	}
	endvnode = e_vnodebase + numvnodes;
	(void)printf("%d active vnodes\n", numvnodes);


#define ST	mp->mnt_stat
	maddr = NULL;
	mp = NULL;	/* XXX just for gcc -Wall */
	for (evp = e_vnodebase; evp < endvnode; evp++) {
		vp = &evp->vnode;
		if (vp->v_mount != maddr) {
			/*
			 * New filesystem
			 */
			if ((mp = getmnt(vp->v_mount)) == NULL)
				continue;
			maddr = vp->v_mount;
			mount_print(mp);
			vnode_header();
			if (!strcmp(ST.f_fstypename, "ufs") ||
			    !strcmp(ST.f_fstypename, "mfs"))
				ufs_header();
			else if (!strcmp(ST.f_fstypename, "nfs"))
				nfs_header();
			else if (!strcmp(ST.f_fstypename, "union"))
				union_header();
			else if (!strcmp(ST.f_fstypename, "cd9660"))
				iso_header();
			else
				continue;
			(void)printf("\n");
		}
		vnode_print(evp->avnode, vp);
		if (!strcmp(ST.f_fstypename, "ufs") ||
		    !strcmp(ST.f_fstypename, "mfs"))
			ufs_print(vp);
		else if (!strcmp(ST.f_fstypename, "nfs"))
			nfs_print(vp);
		else if (!strcmp(ST.f_fstypename, "union"))
			(void) union_print(vp);
		else if (!strcmp(ST.f_fstypename, "cd9660"))
			iso_print(vp);
		else
			continue;
		(void)printf("\n");
	}
	free(e_vnodebase);
}

void
vnode_header()
{
	(void)printf("ADDR     TYP VFLAG  USE HOLD");
}

void
vnode_print(avnode, vp)
	struct vnode *avnode, *vp;
{
	static struct typemap tm[] = {
		{ VBAD, "bad" },
		{ VBLK, "blk" },
		{ VCHR, "chr" },
		{ VDIR, "dir" },
		{ VFIFO, "fif" },
		{ VLNK, "lnk" },
		{ VNON, "non" },
		{ VREG, "reg" },
		{ VSOCK, "soc" },
		{ -1, "unk" }
	};
	static struct flagmap fm[] = {
		{ VV_ALIASED, 'A' },
		{ VI_BWAIT, 'B' },
		{ VV_ISTTY, 'I' },
		{ VV_ROOT, 'R' },
		{ VV_SYSTEM, 'S' },
		{ VV_TEXT, 'T' },
		{ VI_XLOCK, 'L' },
		{ VI_XWANT, 'W' },
		{ 0, 0 }
	};

	(void)printf("%8lx %s %5s %4d %4ld", (u_long)avnode,
	    fmttype(vp->v_type, tm), fmtflags(vp->v_vflag | vp->v_iflag, fm, 1),
	    vp->v_usecount, vp->v_holdcnt);
}

void
v_mode(mode_t mode, int type)
{
	char buf[16];		/* at least 11 plus NUL */
	static int vtomode[] = {
		0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR,
		S_IFLNK, S_IFSOCK, S_IFIFO, 0
	};

	if ((u_int)type <= sizeof vtomode / sizeof vtomode[0])
		mode |= vtomode[type];
	strmode(mode, buf);
	printf(" %10s", buf);
}

void
v_uid(uid)
	uid_t uid;
{

	printf(" %12.12s", user_from_uid(uid, 0));
}

void
ufs_header() 
{
	(void)printf(" FILEID IFLAG       MODE          UID   RDEV|SZ");
}

void
ufs_print(vp) 
	struct vnode *vp;
{
	struct inode inode, *ip = &inode;
	static struct flagmap fm[] = {
		{ IN_ACCESS, 'A' },
		{ IN_CHANGE, 'C' },
		{ IN_EXLOCK, 'E' },
		{ IN_MODIFIED, 'M' },
		{ IN_RENAME, 'R' },
		{ IN_SHLOCK, 'S' },
		{ IN_UPDATE, 'U' },
		{ 0, 0 }
	};

	KGETRETVOID(vp->v_data, &inode, sizeof(struct inode), "vnode's inode");

	(void)printf(" %6ld %5s", ip->i_number, fmtflags(ip->i_flag, fm, 1));
	v_mode(ip->i_mode, vp->v_type);
	v_uid(ip->i_uid);
	if (S_ISCHR(ip->i_mode) || S_ISBLK(ip->i_mode))
		(void)printf(" %7s", fmtdev(ip->i_rdev, ip->i_mode));
	else
		(void)printf(" %7qd", ip->i_size);
}

void
nfs_header() 
{
	(void)printf(" FILEID NFLAG       MODE          UID   RDEV|SZ");
}

void
nfs_print(vp) 
	struct vnode *vp;
{
	struct nfsnode nfsnode, *np = &nfsnode;
	static struct flagmap fm[] = {
		{ NFLUSHINPROG, 'P' },
		{ NFLUSHWANT, 'W' },
		{ NMODIFIED, 'M' },
		{ NQNFSEVICTED, 'G' },
		{ NQNFSNONCACHE, 'X' },
		{ NQNFSWRITE, 'O' },
		{ NWRITEERR, 'E' },
		{ 0, 0 }
	};

	KGETRETVOID(vp->v_data, &nfsnode, sizeof(nfsnode), "vnode's nfsnode");

#define VT	np->n_vattr
	(void)printf(" %6ld %5s", VT.va_fileid, fmtflags(np->n_flag, fm, 1));
	v_mode(VT.va_mode, vp->v_type);
	v_uid(VT.va_uid);
	if (S_ISCHR(VT.va_mode) || S_ISBLK(VT.va_mode))
		(void)printf(" %7s", fmtdev(VT.va_rdev, VT.va_mode));
	else
		(void)printf(" %7qd", np->n_size);
}

void
union_header() 
{
	(void)printf("    UPPER    LOWER");
}

int
union_print(vp) 
	struct vnode *vp;
{
	struct union_node unode, *up = &unode;

	KGETRET(VTOUNION(vp), &unode, sizeof(unode), "vnode's unode");

	(void)printf(" %8x %8x", (u_int)up->un_uppervp, (u_int)up->un_lowervp);
	return (0);
}

/*
 * Given a pointer to a mount structure in kernel space, read it in and
 * return a usable pointer to it.
 */
struct mount *
getmnt(maddr)
	struct mount *maddr;
{
	static struct mtab {
		struct mtab *next;
		struct mount *maddr;
		struct mount mount;
	} *mhead = NULL;
	struct mtab *mt;

	for (mt = mhead; mt != NULL; mt = mt->next)
		if (maddr == mt->maddr)
			return (&mt->mount);
	if ((mt = malloc(sizeof(struct mtab))) == NULL)
		err(1, NULL);
	KGETRET(maddr, &mt->mount, sizeof(struct mount), "mount table");
	mt->maddr = maddr;
	mt->next = mhead;
	mhead = mt;
	return (&mt->mount);
}

void
mount_print(mp)
	struct mount *mp;
{
	int flags;

#define ST	mp->mnt_stat
	(void)printf("*** MOUNT %s %s on %s", ST.f_fstypename,
	    ST.f_mntfromname, ST.f_mntonname);
	if ((flags = mp->mnt_flag)) {
		int i;
		const char *sep = " (";

		for (i = 0; mnt_flags[i].m_flag; i++) {
			if (flags & mnt_flags[i].m_flag) {
				(void)printf("%s%s", sep, mnt_flags[i].m_name);
				flags &= ~mnt_flags[i].m_flag;
				sep = ",";
			}
		}
		if (flags)
			(void)printf("%sunknown_flags:%x", sep, flags);
		(void)printf(")");
	}
	(void)printf("\n");
#undef ST
}

char hdr[] = "    LINE RAW CAN OUT  HWT LWT    COL STATE   SESS  PGID DISC\n";

int
td_compare(arg1, arg2)
	const void *arg1, *arg2;
{
	const struct ttydevice_tmp *p1 = arg1, *p2 = arg2;

	if (p1->tty_major != p2->tty_major)
		return (p1->tty_major - p2->tty_major);
	if (p1->tty_unit != p2->tty_unit)
		return (p1->tty_unit - p2->tty_unit);
	return (p1->tty_base - p2->tty_base);
}

#define HIGHBIT 0x10000

void
ttymode()
{
	int ntds, ntts, i, j, tcount;
	struct ttystat *ts;
	struct ttydevice_tmp *td;

	if (kvm_ttys(kd, &td, &ntds, &ts, &ntts) < 0)
		errx(1, "kvm_ttys: %s", kvm_geterr(kd));

	for (tcount = 0, i = 0; i < ntds; i++) {
		/*
		 * Save the location of the first ttystat 
		 * structure associated with each ttydevice_tmp.
		 */
		td[i].tty_ttys = (struct tty *)&ts[tcount];
		tcount += td[i].tty_count;
		/*
		 * Rewrite major numbers so that they'll sort
		 * into the order that they are first found in
		 * the table.
		 */
		if ((td[i].tty_major & HIGHBIT) == 0) {
			for (j = i + 1; j < ntds; j++) {
				if (td[j].tty_major == td[i].tty_major)
					td[j].tty_major = HIGHBIT | i;
			}
			td[i].tty_major = HIGHBIT | i;
		}
	}

	qsort(td, ntds, sizeof(*td), td_compare);

	while (ntts > 0 && ntds > 0) {
		tcount = td->tty_count;
		for (i = 1; i < ntds; ++i)
			if (strcmp(td[i].tty_name, td->tty_name) == 0)
				tcount += td[i].tty_count;
			else
				break;

		printf("%d %s line%s\n", tcount, td->tty_name,
		    tcount != 1 ? "s" : "");

		if (tcount <= 0) {
			++td;
			--ntds;
			continue;
		}

		printf(hdr);
		ts = (struct ttystat *)td->tty_ttys;
		while (tcount-- > 0 && ntts-- > 0) {
			while (td->tty_count-- <= 0) {
				++td;
				--ntds;
				ts = (struct ttystat *)td->tty_ttys;
			}
			ttyprt(ts++, td->tty_base++);
		}
		++td;
		--ntds;
	}
}

void
ttyprt(ts, line)
	struct ttystat *ts;
	int line;
{
	char *name;
	static struct flagmap fm[] = {
		{ TS_ASLEEP,	'A' },
		{ TS_ASYNC,	'Y' },
		{ TS_BKSL,	'D' },
		{ TS_BUSY,	'B' },
		{ TS_CARR_ON,	'C' },
		{ TS_CNTTB,	'N' },
		{ TS_ERASE,	'E' },
		{ TS_ESLEEP,	'Z' },
		{ TS_FLUSH,	'F' },
		{ TS_ISOPEN,	'O' },
		{ TS_LNCH,	'L' },
		{ TS_TBLOCK,	'K' },
		{ TS_TIMEOUT,	'T' },
		{ TS_TTSTOP,	'S' },
		{ TS_TYPEN,	'P' },
		{ TS_WOPEN,	'W' },
		{ TS_XCLUDE,	'X' },
		{ 0, 0 }
	};
#define TS_NOT_PRINTED (TS_RWAIT | TS_LOCK | TS_XON_PEND | TS_XOFF_PEND)

	if (usenumflag || ts->ts_dev == 0 ||
	    (name = devname(ts->ts_dev, S_IFCHR)) == NULL)
		(void)printf("%8d ", line);
	else
		(void)printf("%8s ", name);
	(void)printf("%3d %3d ", ts->ts_rawqcc, ts->ts_canqcc);
	(void)printf("%3d %4d %3d %6d ", ts->ts_outqcc, 
		ts->ts_hiwat, ts->ts_lowat, ts->ts_column);
	/* 
	 * For compatibility with old kernels, use the default value of KERNBASE
	 * if the symbol does not exist.
	*/
	(void)printf("%-5s %6lx",
	    fmtflags(ts->ts_state & ~TS_NOT_PRINTED, fm, 1),
	    ts->ts_session ? (u_long)ts->ts_session - 
		     (nl[VM_KERNBASE].n_type == 0 ? __KERNBASE : nl[VM_KERNBASE].n_value) 
		     : 0);
	(void)printf(" %5d ", ts->ts_pgid);
	switch (ts->ts_line) {
	case TTYDISC:
		(void)printf("term\n");
		break;
	case TABLDISC:
		(void)printf("tablet\n");
		break;
	case SLIPDISC:
		(void)printf("slip\n");
		break;
	case PPPDISC:
		(void)printf("ppp\n");
		break;
	default:
		(void)printf("%d\n", ts->ts_line);
		break;
	}
}

void
filemode()
{
	struct file *fp, *fbuf;
	int i, maxfile, nfile;
	static char *dtypes[] = { "???", "inode", "socket" };
	static struct flagmap fm[] = {
		{ FAPPEND, 'A' },
		{ FASYNC, 'I' },
		{ FREAD, 'R' },
		{ FWRITE, 'W' },
#ifdef FSHLOCK	/* currently gone */
		{ FEXLOCK, 'X' },
		{ FSHLOCK, 'S' },
#endif
		{ 0, 0 }
	};

	KGET(FNL_MAXFILE, maxfile);
	if (totalflag) {
		KGET(FNL_NFILE, nfile);
		(void)printf("%3d/%3d files\n", nfile, maxfile);
		return;
	}
	if ((fbuf = kvm_files(kd, &nfile)) == NULL) {
		warnx("kvm_files: %s", kvm_geterr(kd));
		return;
	}
	(void)printf("%d/%d open files\n", nfile, maxfile);
	/* kvm_files whacks the f_list.le_next pointers to be self-pointers. */
	(void)printf("   LOC   TYPE    FLG     CNT  MSG    DATA    OFFSET\n");
	for (fp = fbuf, i = 0; i < nfile; fp++, i++) {
		if ((unsigned)fp->f_type > DTYPE_SOCKET)
			continue;
		(void)printf("%8lx ", (u_long)fp->f_list.le_next);
		(void)printf("%-8.8s", dtypes[fp->f_type]);
		(void)printf("%6s  %3d",
		    fmtflags(fp->f_flag, fm, 0), fp->f_count);
		(void)printf("  %3d", fp->f_msgcount);
		(void)printf("  %8.1lx", (u_long)fp->f_data);
		if (fp->f_offset < 0)
			(void)printf("  %qx\n", fp->f_offset);
		else
			(void)printf("  %qd\n", fp->f_offset);
	}
	free(fbuf);
}

#ifdef __bsdi__
void
swapinfo()
{
	struct swapstats swap;
	struct swdevt *sw, *swlist;
	char *header;
	int hlen, i, nblks;
	long bs, used;
	size_t size;

	KGET(VM_SWAPSTATS, swap);
	header = getbsize(&hlen, &bs);
	used = swap.swap_total - swap.swap_free;
	if (totalflag) {
		printf("%d/%d swap space (%s)\n",
		    (int)((quad_t)used * DEV_BSIZE / bs),
		    (int)((quad_t)swap.swap_total * DEV_BSIZE / bs),
		    header);
		return;
	}

	size = swap.swap_nswdev * sizeof *sw;
	if ((sw = malloc(size)) == NULL)
		err(1, "malloc(%lu)", (u_long)size);

	KGET(VM_SWDEVLIST, swlist);
	(void)printf("%-12s %*s  %s\n", "Device name", hlen, header, "Type");
	/*
	 * We don't actually use the array of swdevt structs,
	 * we only look at one entry at a time...
	 */
	for (i = 0; i < swap.swap_nswdev && swlist; i++) {
		KGET2(swlist, sw + i, sizeof(*sw), "swdevt");
		if (sw[i].sw_dev == NODEV)	/* "cannot happen" */
			break;
		(void)printf("%-12s ", fmtdev(sw[i].sw_dev, S_IFBLK));
		/* the first ctod(CLSIZE) is neither used nor free */
		nblks = sw[i].sw_nblks;
		if (nblks >= ctod(CLSIZE))
			nblks -= ctod(CLSIZE);
		(void)printf("%*d  %s\n", hlen,
			(int)((u_quad_t)nblks * DEV_BSIZE / bs),
			(sw[i].sw_flags & SW_FREED) ?
			((sw[i].sw_flags & SW_SEQUENTIAL) ?
			     "Sequential" : "Interleaved") :
			"*** not currently available for swapping ***");
		swlist = sw[i].sw_next;
	}
	(void)printf("%d (%s) allocated out of %d (%s) total, %.0f%% in use\n",
	    (int)((quad_t)used * DEV_BSIZE / bs), header,
	    (int)((quad_t)swap.swap_total * DEV_BSIZE / bs), header,
	    swap.swap_total > 0 ?
	        (double)used / (double)swap.swap_total * 100.0 : 0.0);
}
#else
/*
 * This is based on a program called swapinfo written
 * by Kevin Lahey <kml@rokkaku.atl.ga.us>.
 */
void
swapinfo()
{
	char *header;
	int hlen, nswap, nswdev, dmmax, nswapmap, niswap, niswdev;
	int s, e, div, i, l, avail, nfree, npfree, used;
	struct swdevt *sw;
	long blocksize, *perdev;
	struct map *swapmap, *kswapmap;
	struct mapent *mp;

	KGET(VM_NSWAP, nswap);
	KGET(VM_NSWDEV, nswdev);
	KGET(VM_DMMAX, dmmax);
	KGET(VM_NSWAPMAP, nswapmap);
	KGET(VM_SWAPMAP, kswapmap);	/* kernel `swapmap' is a pointer */
	if ((sw = malloc(nswdev * sizeof(*sw))) == NULL ||
	    (perdev = malloc(nswdev * sizeof(*perdev))) == NULL ||
	    (mp = malloc(nswapmap * sizeof(*mp))) == NULL)
		err(1, "malloc");
	KGET1(VM_SWDEVT, sw, nswdev * sizeof(*sw), "swdevt");
	KGET2((long)kswapmap, mp, nswapmap * sizeof(*mp), "swapmap");

	/* Supports sequential swap */
	if (nl[VM_NISWAP].n_value != 0) {
		KGET(VM_NISWAP, niswap);
		KGET(VM_NISWDEV, niswdev);
	} else {
		niswap = nswap;
		niswdev = nswdev;
	}

	/* First entry in map is `struct map'; rest are mapent's. */
	swapmap = (struct map *)mp;
	if (nswapmap != swapmap->m_limit - (struct mapent *)kswapmap)
		errx(1, "panic: nswapmap goof");

	/* Count up swap space. */
	nfree = 0;
	memset(perdev, 0, nswdev * sizeof(*perdev));
	for (mp++; mp->m_addr != 0; mp++) {
		s = mp->m_addr;			/* start of swap region */
		e = mp->m_addr + mp->m_size;	/* end of region */
		nfree += mp->m_size;

		/*
		 * Swap space is split up among the configured disks.
		 *
		 * For interleaved swap devices, the first dmmax blocks
		 * of swap space some from the first disk, the next dmmax
		 * blocks from the next, and so on up to niswap blocks.
		 *
		 * Sequential swap devices follow the interleaved devices
		 * (i.e. blocks starting at niswap) in the order in which
		 * they appear in the swdev table.  The size of each device
		 * will be a multiple of dmmax.
		 *
		 * The list of free space joins adjacent free blocks,
		 * ignoring device boundries.  If we want to keep track
		 * of this information per device, we'll just have to
		 * extract it ourselves.  We know that dmmax-sized chunks
		 * cannot span device boundaries (interleaved or sequential)
		 * so we loop over such chunks assigning them to devices.
		 */
		i = -1;
		while (s < e) {		/* XXX this is inefficient */
			int bound = roundup(s+1, dmmax);

			if (bound > e)
				bound = e;
			if (bound <= niswap) {
				/* Interleaved swap chunk. */
				if (i == -1)
					i = (s / dmmax) % niswdev;
				perdev[i] += bound - s;
				if (++i >= niswdev)
					i = 0;
			} else {
				/* Sequential swap chunk. */
				if (i < niswdev) {
					i = niswdev;
					l = niswap + sw[i].sw_nblks;
				}
				while (s >= l) {
					/* XXX don't die on bogus blocks */
					if (i == nswdev-1)
						break;
					l += sw[++i].sw_nblks;
				}
				perdev[i] += bound - s;
			}
			s = bound;
		}
	}

	header = getbsize(&hlen, &blocksize);
	if (!totalflag)
		(void)printf("%-11s %*s %8s %8s %8s  %s\n",
		    "Device", hlen, header,
		    "Used", "Avail", "Capacity", "Type");
	div = blocksize / 512;
	avail = npfree = 0;
	for (i = 0; i < nswdev; i++) {
		int xsize, xfree;
		char *name;

		if (!totalflag) {
			if ((name = devname(sw[i].sw_dev, S_IFBLK)) == NULL)
				name = "??";
			(void)printf("/dev/%-6s %*d ",
			    name, hlen, sw[i].sw_nblks / div);

		/*
		 * Don't report statistics for partitions which have not
		 * yet been activated via swapon(8).
		 */
		if (!(sw[i].sw_flags & SW_FREED)) {
			if (totalflag)
				continue;
			(void)printf(" *** not available for swapping ***\n");
			continue;
		}
		xsize = sw[i].sw_nblks;
		xfree = perdev[i];
		used = xsize - xfree;
		npfree++;
		avail += xsize;
		if (totalflag)
			continue;
		(void)printf("%8d %8d %5.0f%%    %s\n", 
		    used / div, xfree / div,
		    (double)used / (double)xsize * 100.0,
		    (sw[i].sw_flags & SW_SEQUENTIAL) ?
			     "Sequential" : "Interleaved");
	}

	/* 
	 * If only one partition has been set up via swapon(8), we don't
	 * need to bother with totals.
	 */
	used = avail - nfree;
	if (totalflag) {
		(void)printf("%dM/%dM swap space\n", used / 2048, avail / 2048);
		return;
	}
	if (npfree > 1) {
		(void)printf("%-11s %*d %8d %8d %5.0f%%\n",
		    "Total", hlen, avail / div, used / div, nfree / div,
		    (double)used / (double)avail * 100.0);
	}
}
#endif

void
usage()
{
	(void)fprintf(stderr,
	    "usage: pstat -Tfnstv [system] [-M core] [-N system]\n");
	exit(1);
}
