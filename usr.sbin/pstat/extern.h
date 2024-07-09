/*-
 * Copyright (c) 1994 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI extern.h,v 2.1 1995/02/03 17:53:58 polk Exp
 */

struct flagmap {
	int	fm_flag;		/* flag bit(s) */
	int	fm_char;		/* char representing it/them */
};
struct typemap {
	int	tm_type;		/* type value (-1 = default) */
	char	*tm_name;		/* corresponding name */
};

kvm_t	*kd;

void	iso_header __P((void));
void	iso_print __P((struct vnode *));
char	*fmtdev __P((dev_t, int));
char	*fmtflags __P((int, struct flagmap *, int));
char	*fmttype __P((int, struct typemap *));
void	v_mode __P((mode_t, int));
void	v_uid __P((uid_t));

#define	KGETRETVOID(addr, p, s, msg)					\
	if (kvm_read(kd, (u_long)(addr), p, s) != s)			\
		return
