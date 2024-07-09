/*	BSDI config.h,v 1.1 1998/12/23 20:24:41 prb Exp	*/

/*
 * For emulation purposes, we relocate the various 1.9.5 config files.
 */

#include "../../ld.so-1.9.5/D/config.h"

#undef LDSO_IMAGE
#undef LDSO_CONF
#undef LDSO_CACHE
#undef LDSO_PRELOAD

#define	LDSO_IMAGE	"/linux/lib/ld-linux.so.1"
#define	LDSO_CONF	"/linux/etc/ld.so.conf.1"
#define	LDSO_CACHE	"/linux/etc/ld.so.cache.1"
#define	LDSO_PRELOAD	"/linux/etc/ld.so.preload.1"
