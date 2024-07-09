/*	BSDI directory.x,v 1.5 2001/06/24 16:51:25 donn Exp	*/

/*
 * The directory(3) routines.
 */

include "types.xh"
include "dirent.xh"

%{
int __bsdi_closedir(DIR *);
DIR *__bsdi_opendir(const char *);
struct dirent *__bsdi_readdir(DIR *);
void __bsdi_rewinddir(DIR *);
void __bsdi_seekdir(DIR *, long);
long __bsdi_telldir(const DIR *);

void *__libc_realloc(void *, size_t);
%}

typedef struct _dirdesc DIR;

int closedir(DIR *dirp) { return (__bsdi_closedir(dirp)); }

/*
 * For opendir(), we reallocate the buffer to twice the normal size and
 * use the second half to hold a Linux dirent structure for readdir().
 */
noerrno DIR *opendir(const char *name)
{
	DIR *dirp;

	if ((dirp = __bsdi_opendir(name)) == NULL) {
		errno = __errno_out(errno);
		return (NULL);
	}
	if ((dirp->dd_buf = __libc_realloc(dirp->dd_buf, 2 * dirp->dd_len)) ==
	    NULL) {
		__bsdi_closedir(dirp);
		errno = __errno_out(ENOMEM);
		return (NULL);
	}
	return (dirp);
}

/*
 * For readdir(), we call the BSD version and convert the BSD dirent
 * into a Linux dirent.
 */
noerrno struct linux_dirent *readdir(DIR *dirp)
{
	struct dirent *dp;
	struct linux_dirent *ldp;
	size_t len;
	int oe;

	if ((dp = __bsdi_readdir(dirp)) == NULL) {
		errno = __errno_out(errno);
		return (NULL);
	}
	ldp = (struct linux_dirent *)(dirp->dd_buf + dirp->dd_len);
	len = dp->d_reclen + (sizeof (*ldp) - sizeof (*dp));
	len = (len + sizeof (linux_off_t) - 1) & ~(sizeof (linux_off_t) - 1);
	dirent_out(dp, ldp, len);
	ldp->linux_d_off = __bsdi_telldir(dirp);
  
	return (ldp);
}

void rewinddir(DIR *dirp) { __bsdi_rewinddir(dirp); }

void seekdir(DIR *dirp, long loc) { __bsdi_seekdir(dirp, loc); }

long telldir(const DIR *dirp) { return (__bsdi_telldir(dirp)); }
