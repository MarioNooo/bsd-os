/*	BSDI statfs.x,v 1.1 1999/04/14 22:51:30 prb Exp	*/

/*
 * Transformation rules for statfs syscalls.
 */

include "statfs.xh"

int statfs(const char *path, struct statfs *buf);
int fstatfs(const int fd, struct statfs *buf);
