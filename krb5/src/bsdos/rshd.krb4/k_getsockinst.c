
#ifndef lint
static char *rcsid_k_getsockinst_c = "/master/kerberosIV/krb/k_getsockinst.c,v 2.1 1996/05/30 01:48:35 jch Exp";
#endif lint

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>

#if defined(KRB5_KRB4_COMPAT)
#include <kerberosIV/krb.h>
#else
#include <des.h>
#endif
#include <krb.h>
#include <netdb.h>

/*
 * Return in inst the name of the local interface bound to socket
 * fd. On Failure return the 'wildcard' instance "*".
 */

int
k_getsockinst(int fd, char *inst)
{
  struct sockaddr_in addr;
  int len = sizeof(addr);
  struct hostent *hnam;
  char *t;

  if (getsockname(fd, (struct sockaddr *)&addr, &len) < 0)
    goto fail;

  hnam = gethostbyaddr((char *)&addr.sin_addr,
		       sizeof(addr.sin_addr),
		       addr.sin_family);
  if (hnam == 0)
    goto fail;

  for (t = hnam->h_name; *t && *t != '.'; t++)
    *inst++ = *t;
  *inst = 0;
  return 0;			/* Success */

 fail:
  inst[0] = '*';
  inst[1] = 0;
  return -1;
}
