
#ifndef lint
static char *rcsid_get_admhst_c =
"/master/kerberosIV/krb/krb_configured.c,v 2.1 1996/08/01 17:48:24 jch Exp";
#endif /* lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>

#if defined(KRB5_KRB4_COMPAT)
#define		KSUCCESS	0
#define		KFAILURE	255
#define		KRB_CONF	"/etc/krb5/krb.conf"

#else
#include <des.h>
#include <krb.h>
#endif

int
krb_configured()
{
	struct stat sb;

	if (stat(KRB_CONF, &sb) < 0)
		return(KFAILURE);

	return(KSUCCESS);
}
