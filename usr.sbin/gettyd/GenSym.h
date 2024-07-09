/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI GenSym.h,v 1.1 1996/05/29 19:30:52 prb Exp
 */
typedef struct GenSym_t {
        char    	*gs_name;
	int		 gs_namelen;
        char    	 gs_flag;
	void 		*gs_value;
	int 		 gs_size;
        struct GenSym_t	*gs_prev;
        struct GenSym_t	*gs_next;
} GenSym_t;

#define	GS_DYNAMIC	0x0001
#define	GS_INSERT	0x0100
#define	GS_REPLACE	0x0300	/* implies GS_INSERT */

#define MalGenSym()     (GenSym_t *)(malloc(sizeof(GenSym_t)))

GenSym_t	*GenSym		(GenSym_t **, char *, int, void *, int, int);
void		 GenSymDelete	(GenSym_t **, char *);
