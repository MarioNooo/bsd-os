/*	BSDI link.h,v 1.1 1997/06/10 23:27:48 donn Exp	*/

#ifndef _LINK_H_
#define	_LINK_H_	1

/* XXX Reconstructed from primitive version in hash.h.  */
struct r_debug{
  int r_version; 
  struct link_map * r_map;
  unsigned long r_brk;
  enum {RT_CONSISTENT, RT_ADD, RT_DELETE} r_state;
  unsigned long r_ldbase;
};

#endif
