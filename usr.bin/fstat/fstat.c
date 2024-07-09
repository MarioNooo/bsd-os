/* BSDI fstat.c,v 2.9 2003/08/20 23:49:54 polk Exp */

/*-
 * Copyright (c) 1988, 1993
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
"@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)fstat.c	8.3 (Berkeley) 5/2/95";
#endif /* not lint */

/*
 * We need the normally kernel-only struct selinfo definition 
 */
#define _NEED_STRUCT_SELINFO

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/ktrace.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/unpcb.h>
#include <sys/sysctl.h>
#include <sys/filedesc.h>
#include <sys/device.h>
#include <sys/disk.h>
#define	KERNEL
#define	_KERNEL
#include <sys/file.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#undef KERNEL
#undef _KERNEL
#define NFS
#include <sys/mount.h>
#include <nfs/nfsproto.h>
#include <nfs/rpcv2.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#undef NFS

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <limits.h>
#include <netdb.h>
#include <nlist.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	TEXT	-1
#define	CDIR	-2
#define	RDIR	-3
#define	TRACE	-4

typedef struct devs {
	struct	devs *next;
	long	fsid;
	ino_t	ino;
	char	*name;
} DEVS;

struct  filestat {
	long	fsid;
	ino_t	fileid;
	mode_t	mode;
	off_t	size;
	dev_t	rdev;
};

static struct nlist nl[] = {
#define	NL_KMEM_MAP	0
	{ "_kmem_map" },
	{ "" }
};

static const char *s_header = 
"USER     CMD          PID   FD  DEV    INUM       MODE SZ|DV R/W";
static const char *n_header =
"USER     CMD          PID   FD MOUNT      INUM MODE         SZ|DV R/W";

static int checkfile;		/* true if restricting to particular files or filesystems */
static DEVS *devs;
static int fsflg;		/* show files on same filesystem as file(s) argument */
static kvm_t *kd;
static struct vm_map kmem_map;	/* for verifying kernel addresses */
static int maxfiles;
static int nflg;		/* (numerical) display f.s. and rdev as dev_t */
static struct file **ofiles;	/* buffer of pointers to file structures */
static int pflg;		/* show files open by a particular pid */
static int uflg;		/* show files open by a particular (effective) user */
static int vflg;		/* display errors in locating kernel data objects etc... */

#define dprintf	if (vflg) warnx

#define ALLOC_OFILES(d)	\
	if ((d) > maxfiles) { \
		free(ofiles); \
		ofiles = malloc((d) * sizeof(struct file *)); \
		if (ofiles == NULL) \
			err(1, NULL); \
		maxfiles = (d); \
	}

/*
 * a kvm_read that returns true if everything is read 
 */
#define KVM_READ(kaddr, paddr, len) \
	(kvm_read(kd, (u_long)(kaddr), (char *)(paddr), (len)) == (ssize_t)(len))

/*
 * Verify that a kernel address is in the malloc arena
 */
#define	KA_VALID(a) \
	((a) == NULL || \
	((vm_offset_t)(a) >= kmem_map.header.start && \
	(vm_offset_t)(a) < kmem_map.header.end))

/*
 * Verify that a vnode pointer is valid
 */
#define	VA_VALID(vp) \
	(KA_VALID(vp) && ((vm_offset_t)(vp) & (sizeof(*(vp)) - 1)) == 0)

/*
 * Support for printing a common header
 */
static char	*Uname, *Comm;
static int	Pid;

#define PREFIX(i) printf("%-8.8s %-10s %5d", Uname, Comm, Pid); \
	switch(i) { \
	case TEXT: \
		printf(" text"); \
		break; \
	case CDIR: \
		printf("   wd"); \
		break; \
	case RDIR: \
		printf(" root"); \
		break; \
	case TRACE: \
		printf("   tr"); \
		break; \
	default: \
		printf(" %4d", i); \
		break; \
	}

static int cd9660_filestat(struct vnode *, struct filestat *);
static void dofiles(struct kinfo_proc *);
static int getfname(char *);
static char *getmnton(struct mount *);
static int nfs_filestat(struct vnode *, struct filestat *);
static void socktrans(struct socket *sock, int i);
static int ufs_filestat(struct vnode *, struct filestat *);
static void usage(void);
static void vtrans(struct vnode *, int, int);

int
main(int argc, char *argv[])
{
	struct kinfo_proc *p, *p0, *p1;
	struct kinfo_proc *pcopy, *plast;
	struct passwd *passwd;
	struct vm_map *kmem_mapp;
	int arg;
	int ch;
	int cnt;
	int rc;
	int what;
	const char *header;
	char *memf;
	char *nlistf;
	char buf[_POSIX2_LINE_MAX];

	arg = 0;
	what = KERN_PROC_ALL;
	nlistf = memf = NULL;
	header = s_header;

	while ((ch = getopt(argc, argv, "fnp:u:vM:N:")) != EOF)
		switch(ch) {
		case 'f':
			fsflg = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			nflg = 1;
			header = n_header;
			break;
		case 'p':
			if (pflg++)
				usage();
			if (!isdigit(*optarg)) {
				warnx("-p requires a process id");
				usage();
			}
			what = KERN_PROC_PID;
			arg = atoi(optarg);
			break;
		case 'u':
			if (uflg++)
				usage();
			if (!(passwd = getpwnam(optarg)))
				errx(1, "unknown uid: %s", optarg);
			what = KERN_PROC_UID;
			arg = passwd->pw_uid;
			break;
		case 'v':
			vflg = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (*argv != NULL) {
		for (; *argv != NULL; ++argv) {
			if (getfname(*argv))
				checkfile = 1;
		}
		if (!checkfile)	/* file(s) specified, but none accessable */
			exit(1);
	}

	ALLOC_OFILES(FD_SETSIZE);	/* reserve space for file pointers */

	if (fsflg && !checkfile) {	
		/* -f with no files means use wd */
		if (getfname(".") == 0)
			exit(1);
		checkfile = 1;
	}

	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL)
		setgid(getgid());

	/* Open and initialize */
	if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == NULL)
		errx(1, "%s", buf);
	if ((rc = kvm_nlist(kd, nl)) < 0)
		errx(1, "no namelist: %s", kvm_geterr(kd));
	else if (rc != 0)
		errx(1, "missing namelist symbols");
	if (!KVM_READ(nl[NL_KMEM_MAP].n_value, &kmem_mapp, sizeof(kmem_mapp)) ||
	    !KVM_READ(kmem_mapp, &kmem_map, sizeof(kmem_map)))
		errx(1, "unable to read kmem_map");

	/* Get the list of processes */
	if ((p0 = kvm_getprocs(kd, what, arg, &cnt)) == NULL)
		errx(1, "%s", kvm_geterr(kd));

	/* Fail if no processes were returned */
	if (cnt == 0) {
		errno = ESRCH;
		err(1, NULL);
	}

	/* If a specific target, complain if it is a zombie */
	if (pflg &&
	    (p0->kp_proc.p_stat == SZOMB || p0->kp_proc.p_flag & P_WEXIT)) {
		errx(1, "pid %d is %s", arg,
		    p0->kp_proc.p_stat == SZOMB ? "a zombie" : "exiting");
	}

	/* Print the header */
	printf("%s%s\n", header,
	    (checkfile && fsflg == 0) ? " NAME" : "");

	/* Handle dump for single process */
	if (pflg) {
		dofiles(p0);
		exit(0);
	}
		
	/*
	 * To insure that a process still exists, fetch its
	 * information again just before scanning its files.
	 */
	if ((pcopy = malloc(cnt * sizeof(*p0))) == NULL)
		err(1, NULL);

	memcpy(pcopy, p0, cnt * sizeof(*p0));
	for (p = pcopy, plast = &p[cnt]; p < plast; ++p) {
		p1 = kvm_getprocs(kd, KERN_PROC_PID, p->kp_proc.p_pid,
		    &cnt);
		/* 
		 * Ignore it if it no longer exists, is a
		 * zombie, or is exiting.
		 */
		if (p1 == NULL || cnt == 0 ||
		    p->kp_proc.p_stat == SZOMB || 
		    p->kp_proc.p_flag & P_WEXIT)
			continue;
		dofiles(p1);
	}
	free(pcopy);

	exit(0);
}

/*
 * print open files attributed to this process
 */
static void
dofiles(struct kinfo_proc *kp)
{
	struct file file;
	struct filedesc0 filed0;
#define	filed	filed0.fd_fd
	struct proc *p;
	struct eproc *ep;
	u_int temp;
	int i;

	p = &kp->kp_proc;
	ep = &kp->kp_eproc;

	/* Set the global fields for printing the common header */
	Uname = user_from_uid(ep->e_cred.pc_ucred.cr_uid, 0);
	Pid = p->p_pid;
	Comm = p->p_comm;

	/* Give up if there are no file descriptors */
	if (p->p_fd == NULL)
		return;

	if (!KVM_READ(p->p_fd, &filed0, sizeof (filed0))) {
		dprintf("can not read filedesc at %p for pid %d",
			p->p_fd, Pid);
		return;
	}

#define	POWER_OF_TWO(v)	(((temp = (v)) & (temp - 1)) == 0)
	/* Sanity check filedesc */
	if (
	    /* Verify vnode pointers */
	    !VA_VALID(filed.fd_rdir) ||
	    !VA_VALID(filed.fd_cdir) ||
	    !VA_VALID(p->p_tracep) ||
	    /* Verify file descriptor numbers */
	    filed.fd_freefile > filed.fd_lastfile + 1 ||
	    filed.fd_lastfile >= filed.fd_nfiles ||
	    /* Verify pointers into filedesc0 */
	    filed.fd_nfiles < NDFILE ||
	    (filed.fd_nfiles == NDFILE &&
		(filed.fd_ofiles != ((struct filedesc0 *)p->p_fd)->fd_dfiles ||
		    filed.fd_ofileflags !=
		    ((struct filedesc0 *)p->p_fd)->fd_dfileflags)) ||
	    /* Verify pointers outside of filedesc0 and value of fd_nfiles */
	    (filed.fd_nfiles > NDFILE &&
		(!VA_VALID(filed.fd_ofiles) ||
		    !VA_VALID(filed.fd_ofileflags) ||
		    !POWER_OF_TWO(filed.fd_nfiles / NDEXTENT)))) {
		dprintf("filedesc sanity check failed for pid %u",
			Pid);
		return;
	}

	/*
	 * root directory vnode, if one
	 */
	if (filed.fd_rdir != NULL)
		vtrans(filed.fd_rdir, RDIR, FREAD);

	/*
	 * current working directory vnode
	 */
	vtrans(filed.fd_cdir, CDIR, FREAD);

	/*
	 * ktrace vnode, if one
	 */
	if (p->p_tracep != NULL)
		vtrans(p->p_tracep->kc_vnode, TRACE, FREAD|FWRITE);

	/*
	 * open files
	 */
#define FPSIZE	(sizeof (struct file *))
	ALLOC_OFILES(filed.fd_lastfile+1);
	if (filed.fd_ofiles != ((struct filedesc0 *)p->p_fd)->fd_dfiles) {
		/* Read external ofiles array */
		if (!KVM_READ(filed.fd_ofiles, ofiles,
		    (filed.fd_lastfile+1) * FPSIZE)) {
			dprintf("can not read file structures at %p for pid %d",
			    filed.fd_ofiles, Pid);
			return;
		}
	} else
		bcopy(filed0.fd_dfiles, ofiles, (filed.fd_lastfile+1) * FPSIZE);

	setprotoent(1);
	for (i = 0; i <= filed.fd_lastfile; i++) {
		if (ofiles[i] == NULL)
			continue;
		if (!KVM_READ(ofiles[i], &file, sizeof (struct file))) {
			dprintf("can not read file %d at %p for pid %d",
				i, ofiles[i], Pid);
			continue;
		}
		if (file.f_type == DTYPE_VNODE) {
			if (file.f_data != NULL)
				vtrans((struct vnode *)file.f_data, i, file.f_flag);
		} else if (file.f_type == DTYPE_SOCKET) {
			if (checkfile == 0)
				socktrans((struct socket *)file.f_data, i);
		}
		else {
			dprintf("unknown file type %d for file %d of pid %d",
				file.f_type, i, Pid);
		}
	}
	endprotoent();
}

static void
vtrans(struct vnode *vp, int i, int flag)
{
	struct vnode vn;
	struct filestat fst;
	char *badtype;
	char *filename;
	char mode[15];
	char rw[3];

	filename = badtype = NULL;
	if (!KVM_READ(vp, &vn, sizeof (struct vnode))) {
		dprintf("can not read vnode at %p for pid %d",
			vp, Pid);
		return;
	}
	if (vn.v_type == VNON || vn.v_tag == VT_NON)
		badtype = "none";
	else if (vn.v_type == VBAD)
		badtype = "bad";
	else
		switch (vn.v_tag) {
		case VT_UFS:
			if (!ufs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_MFS:
			if (!ufs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_NFS:
			if (!nfs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_ISOFS:
			if (!cd9660_filestat(&vn, &fst))
				badtype = "error";
			break;
		default: {
			static char unknown[10];
			sprintf(badtype = unknown, "?(%x)", vn.v_tag);
			break;
		}
	}
	if (checkfile) {
		int fsmatch = 0;
		DEVS *d;

		if (badtype != NULL)
			return;
		for (d = devs; d != NULL; d = d->next)
			if (d->fsid == fst.fsid) {
				fsmatch = 1;
				if (d->ino == fst.fileid) {
					filename = d->name;
					break;
				}
			}
		if (fsmatch == 0 || (filename == NULL && fsflg == 0))
			return;
	}
	PREFIX(i);
	if (badtype != NULL) {
		(void)printf(" -         -  %10s    -\n", badtype);
		return;
	}
	if (nflg)
		printf(" %2d,%d,%d", major(fst.fsid),
		    dk_unit(fst.fsid), dk_part(fst.fsid));
	else
		(void)printf(" %-8s", getmnton(vn.v_mount));
	if (nflg)
		(void)sprintf(mode, "%o", fst.mode);
	else
		strmode(fst.mode, mode);
	(void)printf(" %6lu %10s", fst.fileid, mode);
	switch (vn.v_type) {
		char *name;

	case VBLK:
	case VCHR:
		if (nflg || ((name = devname(fst.rdev, vn.v_type == VCHR ? 
		    S_IFCHR : S_IFBLK)) == NULL))
			printf(" %2d,%d,%d", major(fst.rdev),
			    dv_unit(fst.rdev), dv_subunit(fst.rdev));
		else
			printf(" %6s", name);
		break;
	default:
		printf(" %6qd", fst.size);
		break;
	}
	rw[0] = '\0';
	if (flag & FREAD)
		strcat(rw, "r");
	if (flag & FWRITE)
		strcat(rw, "w");
	printf(" %2s", rw);
	if (filename != NULL && !fsflg)
		printf("  %s", filename);
	printf("\n");
}

static int
ufs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct inode inode;

	if (!KVM_READ(VTOI(vp), &inode, sizeof (inode))) {
		dprintf("can not read inode at %p for pid %d",
			VTOI(vp), Pid);
		return (0);
	}
	fsp->fsid = inode.i_dev;
	fsp->fileid = inode.i_number;
	fsp->mode = (mode_t)inode.i_mode;
	fsp->size = (off_t)inode.i_size;
	fsp->rdev = inode.i_rdev;

	return (1);
}

static int
nfs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct nfsnode nfsnode;
	mode_t mode;

	if (!KVM_READ(VTONFS(vp), &nfsnode, sizeof (nfsnode))) {
		dprintf("can not read nfsnode at %p for pid %d",
			VTONFS(vp), Pid);
		return (0);
	}
	fsp->fsid = nfsnode.n_vattr.va_fsid;
	fsp->fileid = nfsnode.n_vattr.va_fileid;
	fsp->size = nfsnode.n_size;
	fsp->rdev = nfsnode.n_vattr.va_rdev;
	mode = (mode_t)nfsnode.n_vattr.va_mode;
	switch (vp->v_type) {
	case VREG:
		mode |= S_IFREG;
		break;
	case VDIR:
		mode |= S_IFDIR;
		break;
	case VBLK:
		mode |= S_IFBLK;
		break;
	case VCHR:
		mode |= S_IFCHR;
		break;
	case VLNK:
		mode |= S_IFLNK;
		break;
	case VSOCK:
		mode |= S_IFSOCK;
		break;
	case VFIFO:
		mode |= S_IFIFO;
		break;
	};
	fsp->mode = mode;

	return (1);
}


static char *
getmnton(struct mount *m)
{
	static struct mount mount;
	static struct mtab {
		struct mtab *next;
		struct mount *m;
		char mntonname[MNAMELEN];
	} *mhead;
	struct mtab *mt;

	mhead = NULL;
	for (mt = mhead; mt != NULL; mt = mt->next)
		if (m == mt->m)
			return (mt->mntonname);
	if (!KVM_READ(m, &mount, sizeof(struct mount))) {
		warnx("can not read mount table at %p", m);
		return (NULL);
	}
	if ((mt = malloc(sizeof (struct mtab))) == NULL)
		err(1, NULL);
	mt->m = m;
	bcopy(&mount.mnt_stat.f_mntonname[0], &mt->mntonname[0], MNAMELEN);
	mt->next = mhead;
	mhead = mt;
	return (mt->mntonname);
}

static void
socktrans(struct socket *sock, int i)
{
	static char *stypename[] = {
		"unused",	/* 0 */
		"stream", 	/* 1 */
		"dgram",	/* 2 */
		"raw",		/* 3 */
		"rdm",		/* 4 */
		"seqpak"	/* 5 */
	};
#define	STYPEMAX 5
	struct socket	so;
	struct protosw	proto;
	struct domain	dom;
	struct inpcb	inpcb;
	struct unpcb	unpcb;
	struct protoent *pp;
	char dname[32];

	PREFIX(i);

	/* fill in socket */
	if (!KVM_READ(sock, &so, sizeof(struct socket))) {
		dprintf("can not read sock at %p", sock);
		goto bad;
	}

	/* fill in protosw entry */
	if (!KVM_READ(so.so_proto, &proto, sizeof(struct protosw))) {
		dprintf("can not read protosw at %p", so.so_proto);
		goto bad;
	}

	/* fill in domain */
	if (!KVM_READ(proto.pr_domain, &dom, sizeof(struct domain))) {
		dprintf("can not read domain at %p", proto.pr_domain);
		goto bad;
	}

	if (!KVM_READ((u_long)dom.dom_name, dname, sizeof(dname) - 1)) {
		dprintf("can not read domain name at %p",
			dom.dom_name);
		dname[0] = '\0';
	}
	else
		dname[sizeof(dname) - 1] = '\0';

	if ((u_short)so.so_type > STYPEMAX)
		printf("* %s ?%d", dname, so.so_type);
	else
		printf("* %s %s", dname, stypename[so.so_type]);

	/* 
	 * protocol specific formatting
	 *
	 * Try to find interesting things to print.  For tcp, the interesting
	 * thing is the address of the tcpcb, for udp and others, just the
	 * inpcb (socket pcb).  For unix domain, its the address of the socket
	 * pcb and the address of the connected pcb (if connected).  Otherwise
	 * just print the protocol number and address of the socket itself.
	 * The idea is not to duplicate netstat, but to make available enough
	 * information for further analysis.
	 */
	switch(dom.dom_family) {
	case AF_INET:
	case AF_INET6:
		if ((pp = getprotobynumber(proto.pr_protocol)) == NULL)
			printf(" %d", proto.pr_protocol);
		else
			printf(" %s", pp->p_name);
		if (proto.pr_protocol == IPPROTO_TCP ) {
			if (so.so_pcb != NULL) {
				if (!KVM_READ((u_long)so.so_pcb,
				    (char *)&inpcb, sizeof(struct inpcb))) {
					dprintf("can not read inpcb at %p",
					    so.so_pcb);
					goto bad;
				}
				printf(" %x", (int)inpcb.inp_ppcb);
			}
		}
		else if (so.so_pcb != NULL)
			printf(" %x", (int)so.so_pcb);
		break;
	case AF_UNIX:
		/* print address of pcb and connected pcb */
		if (so.so_pcb != NULL) {
			printf(" %x", (int)so.so_pcb);
			if (!KVM_READ((u_long)so.so_pcb, (char *)&unpcb,
			    sizeof(struct unpcb))) {
				dprintf("can not read unpcb at %p",
				    so.so_pcb);
				goto bad;
			}
			if (unpcb.unp_conn != NULL) {
				char shoconn[4], *cp;

				cp = shoconn;
				if (!(so.so_state & SS_CANTRCVMORE))
					*cp++ = '<';
				*cp++ = '-';
				if (!(so.so_state & SS_CANTSENDMORE))
					*cp++ = '>';
				*cp = '\0';
				printf(" %s %x", shoconn,
				    (int)unpcb.unp_conn);
			}
		}
		break;
	default:
		/* print protocol number and socket address */
		printf(" %d %x", proto.pr_protocol, (int)sock);
	}
	printf("\n");
	return;

bad:
	printf("* error\n");
}

static int
getfname(char *filename)
{
	struct stat statbuf;
	DEVS *cur;

	if (stat(filename, &statbuf)) {
		warn("%s", filename);
		return (0);
	}
	if ((cur = malloc(sizeof(DEVS))) == NULL)
		err(1, NULL);
	cur->next = devs;
	devs = cur;

	cur->ino = statbuf.st_ino;
	cur->fsid = statbuf.st_dev;
	cur->name = filename;
	return (1);
}

static void
usage(void)
{
	(void)fprintf(stderr,
 "usage: fstat [-fnv] [-p pid] [-u user] [-N system] [-M core] [file ...]\n");
	exit(1);
}

#undef VTOI		/* XXX */
#undef IN_LOCKED	/* XXX */
#undef IN_WANTED	/* XXX */
#undef IN_ACCESS	/* XXX */
#undef i_size		/* XXX */
#include <isofs/cd9660/cd9660_node.h>

static int
cd9660_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct iso_node iso_node;

	if (!KVM_READ(VTOI(vp), &iso_node, sizeof (iso_node))) {
		dprintf("can not read iso_node at %p for pid %d",
			VTOI(vp), Pid);
		return (0);
	}
	fsp->fsid = iso_node.i_dev;
	fsp->fileid = iso_node.i_number;
	fsp->mode = (mode_t)iso_node.inode.iso_mode;
	fsp->size = (off_t)iso_node.i_size;
	fsp->rdev = iso_node.inode.iso_rdev;

	return (1);
}
