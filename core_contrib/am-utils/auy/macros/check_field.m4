dnl ######################################################################
dnl find if structure $1 has field field $2
AC_DEFUN(AC_CHECK_FIELD,
[
# make variable name a concatenation of the structure name and the field
ac_safe=`echo ac_cv_field_$1_$2 | tr '. ' '__'`
ac_upcase_var_name=`echo HAVE_FIELD_$1_$2 | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' | tr '. ' '__'`
AC_CACHE_CHECK_DYNAMIC(if $1 field $2 exist,
$ac_safe,
[
# try to compile a program
AC_TRY_COMPILE(
AC_MOUNT_HEADERS(
[
/* now set the typedef */
#ifdef HAVE_STRUCT_MNTENT
typedef struct mntent mntent_t;
#else /* not HAVE_STRUCT_MNTENT */
# ifdef HAVE_STRUCT_MNTTAB
typedef struct mnttab mntent_t;
# endif /*  HAVE_STRUCT_MNTTAB */
#endif /* not HAVE_STRUCT_MNTENT */

/*
 * for various filesystem specific mount arguments
 */

#ifdef HAVE_SYS_FS_PC_FS_H
# include <sys/fs/pc_fs.h>
#endif /* HAVE_SYS_FS_PC_FS_H */
#ifdef HAVE_MSDOSFS_MSDOSFSMOUNT_H
# include <msdosfs/msdosfsmount.h>
#endif /* HAVE_MSDOSFS_MSDOSFSMOUNT_H */

#ifdef HAVE_SYS_FS_EFS_CLNT_H
# include <sys/fs/efs_clnt.h>
#endif /* HAVE_SYS_FS_EFS_CLNT_H */
#ifdef HAVE_SYS_FS_XFS_CLNT_H
# include <sys/fs/xfs_clnt.h>
#endif /* HAVE_SYS_FS_XFS_CLNT_H */
#ifdef HAVE_SYS_FS_UFS_MOUNT_H
# include <sys/fs/ufs_mount.h>
#endif /* HAVE_SYS_FS_UFS_MOUNT_H */
#ifdef HAVE_SYS_FS_AUTOFS_PROT_H
# include <sys/fs/autofs_prot.h>
#endif /* HAVE_SYS_FS_AUTOFS_PROT_H */
#ifdef HAVE_HSFS_HSFS_H
# include <hsfs/hsfs.h>
#endif /* HAVE_HSFS_HSFS_H */

#ifdef HAVE_IFADDRS_H
# include <ifaddrs.h>
#endif /* HAVE_IFADDRS_H */

]),
[
$1 a;
char *cp = (char *) &(a.$2);
], eval "$ac_safe=yes", eval "$ac_safe=no")
])
if test "`eval echo '$''{'$ac_safe'}'`" = yes
then
  AC_DEFINE_UNQUOTED($ac_upcase_var_name)
fi
])
dnl ======================================================================
