/*
 * Copyright (c) 2000-2001 Sendmail, Inc. and its suppliers.
 *	All rights reserved.
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the sendmail distribution.
 *
 *	path.h,v 1.3 2003/09/17 21:19:19 polk Exp
 */

/*
**  Portable names for standard filesystem paths
**  and macros for directories.
*/

#ifndef SM_PATH_H
# define SM_PATH_H

# include <sm/gen.h>

#  define SM_PATH_DEVNULL	"/dev/null"
#  define SM_IS_DIR_DELIM(c)	((c) == '/')
#  define SM_FIRST_DIR_DELIM(s)	strchr(s, '/')
#  define SM_LAST_DIR_DELIM(s)	strrchr(s, '/')

/* Warning: this must be accessible as array */
#  define SM_IS_DIR_START(s)	((s)[0] == '/')

#  define sm_path_isdevnull(path)	(strcmp(path, "/dev/null") == 0)

#endif /* ! SM_PATH_H */