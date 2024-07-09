/*	BSDI pwd.h,v 1.1 1995/07/10 18:21:42 donn Exp	*/

#ifndef	_SCO_PWD_H_
#define	_SCO_PWD_H_

/* from iBCS2 p6-45 */
struct passwd {
	char	*pw_name;
	char	*pw_passwd;
	int	pw_uid;
	int	pw_gid;
	char	*pw_age;
	char	*pw_comment;
	char	*pw_gecos;
	char	*pw_dir;
	char	*pw_shell;
};

/* prototypes required by POSIX.1 */
#include <sys/cdefs.h>

struct passwd *getpwuid __P((int));
struct passwd *getpwnam __P((const char *));

#endif /* _SCO_PWD_H_ */
