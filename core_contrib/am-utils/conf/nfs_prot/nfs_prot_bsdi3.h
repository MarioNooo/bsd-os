/*
 * Copyright (c) 1997-1999 Erez Zadok
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
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
 *    must display the following acknowledgment:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *      %W% (Berkeley) %G%
 *
 * $Id: nfs_prot_bsdi3.h,v 1.5 1999/03/30 17:22:54 ezk Exp $
 *
 */

#ifndef _AMU_NFS_PROT_H
#define _AMU_NFS_PROT_H

#ifdef HAVE_RPCSVC_NFS_PROT_H
# include <rpcsvc/nfs_prot.h>
#endif /* HAVE_RPCSVC_NFS_PROT_H */
#ifdef HAVE_NFS_NFSV2_H
# include <nfs/nfsv2.h>
#endif /* HAVE_NFS_NFSV2_H */
#ifdef HAVE_NFS_RPCV2_H
# include <nfs/rpcv2.h>
#endif /* HAVE_NFS_RPCV2_H */

#ifndef NFS_NPROCS
# define NFS_NPROCS	26
#endif /* not NFS_NPROCS */
#ifdef HAVE_NFS_NFS_H
# include <nfs/nfs.h>
#endif /* HAVE_NFS_NFS_H */

#ifdef HAVE_SYS_FS_NFS_H
# include <sys/fs/nfs.h>
#endif /* HAVE_SYS_FS_NFS_H */
#ifdef HAVE_RPCSVC_MOUNT_H
# include <rpcsvc/mount.h>
#endif /* HAVE_RPCSVC_MOUNT_H */
#ifdef	HAVE_UFS_UFS_UFSMOUNT_H
# include <ufs/ufs/ufsmount.h>
#endif	/* HAVE_UFS_UFS_UFSMOUNT_H */

/****************************************************************************/
/*
 * NFS V3 support:
 * BSD/OS 3.0 has it but no easy way to figure it out from the header files.
 */
#ifndef MNTTYPE_NFS3
# define MNTTYPE_NFS3	"nfs"
#endif /* MNTTYPE_NFS3 */

#ifndef MOUNTVERS3
# define MOUNTVERS3	((unsigned long)(3))
#endif /* not MOUNTVERS */

#ifndef FHSIZE3
# define FHSIZE3 64
#endif /* not FHSIZE3 */
#ifndef NFS3_FHSIZE
# define NFS3_FHSIZE 64
#endif /* not NFS3_FHSIZE */

typedef struct {
  u_int fhandle3_len;
  char *fhandle3_val;
} fhandle3;

enum mountstat3 {
  MNT_OK = 0,
  MNT3ERR_PERM = 1,
  MNT3ERR_NOENT = 2,
  MNT3ERR_IO = 5,
  MNT3ERR_ACCES = 13,
  MNT3ERR_NOTDIR = 20,
  MNT3ERR_INVAL = 22,
  MNT3ERR_NAMETOOLONG = 63,
  MNT3ERR_NOTSUPP = 10004,
  MNT3ERR_SERVERFAULT = 10006
};
typedef enum mountstat3 mountstat3;

struct mountres3_ok {
  fhandle3 fhandle;
  struct {
    u_int auth_flavors_len;
    int *auth_flavors_val;
  } auth_flavors;
};
typedef struct mountres3_ok mountres3_ok;

struct mountres3 {
  mountstat3 fhs_status;
  union {
    mountres3_ok mountinfo;
  } mountres3_u;
};
typedef struct mountres3 mountres3;

struct nfs_fh3 {
	u_int fh3_length;
	union nfs_fh3_u {
		char data[NFS3_FHSIZE];
	} fh3_u;
};
typedef struct nfs_fh3 nfs_fh3;

extern bool_t xdr_mountres3(XDR *xdrs, mountres3 *objp);
/****************************************************************************/


/*
 * MACROS:
 */
#define	dr_drok_u	diropres
#define ca_where	where
#define da_fhandle	dir
#define da_name		name
#define dl_entries	entries
#define dl_eof		eof
#define dr_status	status
#define dr_u		diropres_u
#define drok_attributes	attributes
#define drok_fhandle	file
#define fh_data		data
#define la_fhandle	from
#define la_to		to
#define na_atime	atime
#define na_ctime	ctime
#define na_fileid	fileid
#define na_fsid		fsid
#define na_mode		mode
#define na_mtime	mtime
#define na_nlink	nlink
#define na_size		size
#define na_type		type
#define ne_cookie	cookie
#define ne_fileid	fileid
#define ne_name		name
#define ne_nextentry	nextentry
#define ns_attr_u	attributes
#define ns_status	status
#define ns_u		attrstat_u
#define nt_seconds	seconds
#define nt_useconds	useconds
#define rda_cookie	cookie
#define rda_count	count
#define rda_fhandle	dir
#define rdr_reply_u	reply
#define rdr_status	status
#define rdr_u		readdirres_u
#define rlr_data_u	data
#define rlr_status	status
#define rlr_u		readlinkres_u
#define rna_from	from
#define rna_to		to
#define rr_status	status
#define sag_fhandle	file
#define sfr_reply_u	reply
#define sfr_status	status
#define sfr_u		statfsres_u
#define sfrok_bavail	bavail
#define sfrok_bfree	bfree
#define sfrok_blocks	blocks
#define sfrok_bsize	bsize
#define sfrok_tsize	tsize
#define sla_from	from
#define wra_fhandle	file


/*
 * TYPEDEFS:
 */
typedef attrstat nfsattrstat;
typedef createargs nfscreateargs;
typedef dirlist nfsdirlist;
typedef diropargs nfsdiropargs;
typedef diropres nfsdiropres;
typedef entry nfsentry;
typedef fattr nfsfattr;
typedef ftype nfsftype;
typedef linkargs nfslinkargs;
typedef readargs nfsreadargs;
typedef readdirargs nfsreaddirargs;
typedef readdirres nfsreaddirres;
typedef readlinkres nfsreadlinkres;
typedef readres nfsreadres;
typedef renameargs nfsrenameargs;
typedef sattrargs nfssattrargs;
typedef statfsokres nfsstatfsokres;
typedef statfsres nfsstatfsres;
typedef symlinkargs nfssymlinkargs;
typedef writeargs nfswriteargs;

/* missing definitions from bsdi4 */
#if defined(HAVE_MAP_HESIOD) && !defined(HAVE_HESIOD_H)
extern int hesiod_init(void **context);
#endif /* defined(HAVE_MAP_HESIOD) && !defined(HAVE_HESIOD_H) */

/*
 * On bsdi4, NFS V3/tcp mounts should not use the default resvport option.
 * Defining this mnt option string will force compute_nfs_args() to not
 * use reserved ports unless it was specified as a mount option.
 */
#ifndef MNTTAB_OPT_RESVPORT
# define MNTTAB_OPT_RESVPORT "resvport"
#endif /* not MNTTAB_OPT_RESVPORT */

#endif /* not _AMU_NFS_PROT_H */
