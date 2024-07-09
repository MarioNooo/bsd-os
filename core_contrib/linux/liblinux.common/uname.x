/*	BSDI uname.x,v 1.4 2003/09/23 17:46:34 donn Exp	*/

/*
 * Transformations for the uname() call.
 */

%{
#include <sys/param.h>
#include <sys/sysctl.h>
%}

include "types.xh"

%{
struct linux_utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char domainname[65];
};
%}

int uname(struct linux_utsname *u)
{
	int mib[2];
	size_t len;
	int r;

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTNAME;
	len = sizeof (u->nodename) - 1;
	r = __bsdi_syscall(SYS___sysctl, mib, 2, &u->nodename, &len, NULL, 0);
	if (__bsdi_error(r) && __bsdi_error_val(r) != ENOMEM)
		return (r);

	mib[0] = CTL_HW;
	mib[1] = HW_MACHINE;
	len = sizeof (u->machine) - 1;
	r = __bsdi_syscall(SYS___sysctl, mib, 2, &u->machine, &len, NULL, 0);
	if (__bsdi_error(r) && __bsdi_error_val(r) != ENOMEM)
		return (r);

	mib[0] = CTL_KERN;
	mib[1] = KERN_NISDOMAINNAME;
	len = sizeof (u->domainname) - 1;
	r = __bsdi_syscall(SYS___sysctl, mib, 2, &u->domainname, &len, NULL, 0);
	if (__bsdi_error(r) && __bsdi_error_val(r) != ENOMEM)
		return (r);

	strcpy(u->sysname, "Linux");
	strcpy(u->release, "2.1.71");
	strcpy(u->version, "BSD/OS LAP");

	return (0);
}
