/*	BSDI stat.h,v 1.1 1995/07/10 18:26:09 donn Exp	*/

#ifndef _SCO_SYS_STAT_H_
#define	_SCO_SYS_STAT_H_

/* from iBCS2 p 6-51 - 6-52 */
struct stat {
	short		st_dev;
	unsigned short	st_ino;
	unsigned short	st_mode;
	short		st_nlink;
	unsigned short	st_uid;
	unsigned short	st_gid;
	short		st_rdev;
	long		st_size;
	long		st_atime;
	long		st_mtime;
	long		st_ctime;
};

#define	S_IFMT	 0170000
#define	S_IFIFO	 0010000
#define	S_IFCHR	 0020000
#define	S_IFDIR	 0040000
#define	S_IFBLK	 0060000
#define	S_IFREG	 0100000
#define	S_IFLNK	 0120000
#define	S_IFSOCK 0140000
#define	S_ISUID	 0004000
#define	S_ISGID	 0002000
#define	S_ISVTX	 0001000

#define	S_IRWXU	0000700
#define	S_IRUSR	0000400
#define	S_IWUSR	0000200
#define	S_IXUSR	0000100
#define	S_IRWXG	0000070
#define	S_IRGRP	0000040
#define	S_IWGRP	0000020
#define	S_IXGRP	0000010
#define	S_IRWXO	0000007
#define	S_IROTH	0000004
#define	S_IWOTH	0000002
#define	S_IXOTH	0000001

#define	S_ISDIR(m)	(((m) & 0170000) == 0040000)
#define	S_ISCHR(m)	(((m) & 0170000) == 0020000)
#define	S_ISBLK(m)	(((m) & 0170000) == 0060000)
#define	S_ISREG(m)	(((m) & 0170000) == 0100000)
#define	S_ISFIFO(m)	(((m) & 0170000) == 0010000)
#define	S_ISLNK(m)	(((m) & 0170000) == 0120000)
#define	S_ISSOCK(m)	(((m) & 0170000) == 0140000)

/* prototypes required by POSIX.1 */

#include <sys/cdefs.h>

int chmod __P((const char *, unsigned int));
int fstat __P((int, struct stat *));
int mkdir __P((const char *, unsigned int));
int mkfifo __P((const char *, unsigned int));
int stat __P((const char *, struct stat *));
unsigned int umask __P((unsigned int));

#endif	/* _SCO_SYS_STAT_H_ */
