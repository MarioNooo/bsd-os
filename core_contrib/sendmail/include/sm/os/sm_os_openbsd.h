/*
 * Copyright (c) 2000 Sendmail, Inc. and its suppliers.
 *	All rights reserved.
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the sendmail distribution.
 *
 *	sm_os_openbsd.h,v 1.3 2003/09/17 21:19:20 polk Exp
 */

/*
**  sm_os_openbsd.h -- platform definitions for OpenBSD
**
**  Note: this header file cannot be called OpenBSD.h
**  because <sys/param.h> defines the macro OpenBSD.
*/

#define SM_OS_NAME	"openbsd"

#define SM_CONF_SYS_CDEFS_H	1
#ifndef SM_CONF_SHM
# define SM_CONF_SHM	1
#endif /* SM_CONF_SHM */
#ifndef SM_CONF_SEM
# define SM_CONF_SEM	1
#endif /* SM_CONF_SEM */
#ifndef SM_CONF_MSG
# define SM_CONF_MSG	1
#endif /* SM_CONF_MSG */
