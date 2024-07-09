/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI extern.h,v 1.2 1996/09/15 20:09:48 bostic Exp
 */

int	 rpasswd(char *user);
int	 rpasswd_close(void);
int	 rpasswd_delete(char *name);
char	*rpasswd_get(char *name);
int	 rpasswd_insert(char *name, char *data);
char	*rpasswd_next(char **data);
void	 rpasswd_rewind(void);
