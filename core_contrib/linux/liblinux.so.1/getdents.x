/*	BSDI getdents.x,v 1.1.1.1 1998/12/23 19:57:20 prb Exp	*/

/*
 * Emulate getdents(), which is like getdirentries() but
 * it doesn't supply a seek pointer.
 */

include "types.xh"
include "dirent.xh"

%{
int __getdirentries(int, struct linux_dirent *, int, long *);
%}

/* Call our getdirentries() emulation.  */
noerrno int getdents(int fd, char *buf, size_t nbytes)
{
	return (__getdirentries(fd, (struct linux_dirent *)buf, (int)nbytes,
	    NULL));
}
