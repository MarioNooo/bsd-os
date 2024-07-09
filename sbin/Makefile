#	BSDI Makefile,v 2.30 2002/08/12 22:40:24 db Exp
#
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# missing: fsdb icheck ncheck
# LFS: dumplfs newlfs 
SUBDIR= asyncd bootparams chkconfig clri disksetup \
	dmesg domainname dump dumpfs frconfig fsck ifconfig init ldconfig \
	mknod mount mount_cd9660 mount_fdesc mount_kernfs mount_lfs \
	mount_loopback mount_msdos mount_nfs mount_portal \
	mount_procfs mount_romfs mount_umap mount_union mountd newfs nfsd \
	nfsiod nologin quotacheck reboot restore route savecore \
	scsicmd shutdown slattach splice startslip swapon sysctl \
	trg tunefs umount vndconfig wv

.if ${MACHINE} == "i386"
SUBDIR+=diskdefect i386
.endif

.include <bsd.subdir.mk>
