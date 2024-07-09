/*	BSDI fcntl.x,v 1.3 1999/04/14 22:32:42 prb Exp	*/

/*
 * Fcntl.h syscall transforms.
 */

include "types.xh"
include "fcntl.xh"

openflags_t fcntl(int fd, F_GETFL, int ignore);
int fcntl(int fd, F_SETFL, openflags_t oflags);
int fcntl(int fd, F_GETLK, struct flock *fl);
int fcntl(int fd, F_SETLK, const struct flock *fl);
int fcntl(int fd, F_SETLKW, const struct flock *fl);
int fcntl(int fd, fcntl_t cmd, int arg);

int open(const char *path, openflags_t flags, mode_t mode);
int flock(int fd, int operation);
