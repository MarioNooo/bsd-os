/*	BSDI dirent.x,v 1.3 2000/12/08 04:21:37 donn Exp	*/

/*
 * Directory transformations.
 */

include "types.xh"
include "dirent.xh"

%{
#include <unistd.h>

struct dirbuf {
	long base;
	int size;
	char buf[4];
};

#define	DIR_SHIM	(sizeof (struct linux_dirent) - sizeof (struct dirent))
%}

/*
 * This transformable structure provides a convenient way to apply
 * transformations across all of the dirent structures in the buffer.
 */
struct dirbuf {
	char linux_buf[4];	/* the length is not actually fixed */
	out(n, f, len) {
		struct dirent *dp;
		struct linux_dirent *fdp;
		int size = 0;

		for (dp = (struct dirent *)n->buf;
		    size < n->size;
		    dp = (struct dirent *)((char *)dp + dp->d_reclen)) {
			fdp = (struct linux_dirent *)&f->linux_buf[size];
			dirent_out(dp, fdp, dp->d_reclen + DIR_SHIM);
			size += dp->d_reclen + DIR_SHIM;
			fdp->linux_d_off = (((char *)dp + dp->d_reclen) -
			    n->buf) + n->base;
		}
	}
};

int getdirentries(int fd, struct dirbuf *dbuf, int length_dbuf, long *basep)
{
	char *buf;
	struct dirent *dp;
	int n_size = length_dbuf - (sizeof (struct dirbuf) -
	    sizeof (struct linux_dirbuf));
	int f_size;
	long off;
	int again;
	int r;

	if (dbuf == NULL)
		/* Force EFAULT.  */
		buf = NULL;
	else
		buf = dbuf->buf;
	if (n_size < 0)
		n_size = 0;

	/*
	 * Loop on getdirentries(), adjusting the buffer size downward
	 * until we read exactly enough dirent structures
	 * to fill the Linux buffer without overflowing.
	 * Note: Short getdirentries() calls are NOT guaranteed to work!
	 * Note: Arbitrary seeks in directories are NOT guaranteed to work!
	 * But glibc does both of these things (sigh), so we may need
	 * to revisit this code.
	 */
	do {
		r = __bsdi_syscall(SYS_getdirentries, fd, buf, n_size, &off);
		if (__bsdi_error(r))
			return (r);

		again = 0;
		f_size = 0;
		for (dp = (struct dirent *)buf;
		    dp < (struct dirent *)&buf[r];
		    dp = (struct dirent *)((char *)dp + dp->d_reclen)) {
			if (f_size + dp->d_reclen + DIR_SHIM > length_dbuf) {
				again = 1;
				__bsdi_qsyscall(SYS_lseek, fd, 0, (off_t)off,
				    SEEK_SET);
				n_size = (char *)dp - buf;
				break;
			}
			f_size += dp->d_reclen + DIR_SHIM;
		}
	} while (again);

	if (basep)
		*basep = dbuf->base;

	/*
	 * Here we pass secret parameters back to the dirbuf output transform.
	 * Linux wants to be able to seek to the value of the last d_off
	 * field and read a valid directory block, so we accommodate it
	 * by making the d_off fields be relative to the ending seek offset.
	 */
	dbuf->base = (long)__bsdi_qsyscall(SYS_lseek, fd, 0, (off_t)0,
	    SEEK_CUR) - r;
	dbuf->size = f_size;

	return (f_size);
}
