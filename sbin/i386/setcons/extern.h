/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI extern.h,v 1.1 1997/01/02 17:38:36 pjd Exp
 */

int	bell __P((int, char *[], int));
void	bell_help __P((void));
int	repeat __P((int, char *[], int));
void	repeat_help __P((void));
int	keymap __P((int, char *[], int));
void	keymap_help __P((void));
