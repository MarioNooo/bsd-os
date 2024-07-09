/*
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI bbops.h,v 2.1 1995/07/25 22:20:02 torek Exp
 */

/*
 * Operations on defect (bad block) lists.
 * Also includes initialization (opendisk, set/get geom, set/get badtype).
 */

/* modifiers for operations: clear, load, print */
#define	CLEAR_OLD	1	/* clear original defect list */
#define	CLEAR_NEW	2	/* clear added defect list */
#define	CLEAR_BOTH	(CLEAR_OLD | CLEAR_NEW)

enum loadhow {
	LOAD_DEFAULT,		/* default method, as appropriate */
	LOAD_MFR,		/* load manufacturer's defect list */
	LOAD_CUR,		/* load current defect list (==default?) */
	LOAD_USER		/* load as specified by user argument */
};
enum printhow {
	PRINT_DEFAULT,		/* print in default format */
	PRINT_CTS,		/* print in cyl/trk/sec format */
	PRINT_BFI,		/* print in cyl/hd/bytes_from_index format */
	PRINT_SN		/* print in absolute sector number format */
};
#define	PRINT_OLD	1	/* print original defect list */
#define	PRINT_NEW	2	/* print new defect list */
#define	PRINT_BOTH	(PRINT_OLD | PRINT_NEW)

/*
 * High level interface.  If an error occurs, most of these print
 * something first.  The exceptions are the three `add bad block'
 * functions: addbfi, addcts, addsn.  Curiously enough, those three
 * are actually macros.
 */
int	opendisk __P((struct ddstate *, char *, int));
void	closedisk __P((struct ddstate *));
int	getgeom __P((struct ddstate *));
int	setgeom __P((struct ddstate *, char *));
int	getbadtype __P((struct ddstate *));
int	setbadtype __P((struct ddstate *, char *));
int	cleardefects __P((struct ddstate *, int));
int	loaddefects __P((struct ddstate *, enum loadhow, char *));
int	setaux __P((struct ddstate *, char *));
void	adjlimits __P((struct ddstate *, daddr_t *, daddr_t *));
int	printdefects __P((struct ddstate *, enum printhow, int));
int	store __P((struct ddstate *));

/*
 * Similar to strerror(), but for add{bfi,cts,sn};
 * decodes overloaded errno's.
 */
char	*str_adderror __P((int));

/*
 * These three `add' functions are implemented as macros.
 */
#define	addbfi(ds, cyl, trk, bfi)				\
	((ds)->d_bb->bb_addbfi(ds, cyl, trk, bfi))

#define	addcts(ds, cyl, trk, sec)				\
	((u_int)(cyl) >= (ds)->d_dl.d_ncylinders ||		\
	 (u_int)(trk) >= (ds)->d_dl.d_ntracks ||		\
	 (u_int)(sec) >= (ds)->d_dl.d_nsectors ? ERANGE :	\
	 (ds)->d_bb->bb_addsn(ds, SN(ds, cyl, trk, sec)))

#define	addsn(ds, sn)						\
	((ds)->d_bb->bb_addsn(ds, sn))
/*
 * Lower level interface to individual bad-block functions is through
 * a per-driver set of vectors:
 *
 *	attach		set up (empty) bad-sector info on given disk
 *	detach		tear down
 *	clear		clear out loaded bad-sector info
 *	load		load existing bad block info from drive
 *	setaux		set auxiliary info from user argument
 *	scanlimit	adjust auto-defect-scan limits as needed
 *	addbfi		add the given <cyl,head,bytes-from-index>
 *	print		print bad block table
 *	store		commit: store modified table to disk
 *
 * Most functions can return errors.  `Load' and `print' take modifiers
 * as to how to do the loading and printing, and load can be asked to
 * load according to a user-specified string.  No disk changes should
 * occur until the final call to store().
 */
struct bbops {
	char	*bb_name;	/* bad block replacement type name */
	char	*bb_longname;	/* long version of above */
	int	(*bb_attach) __P((struct ddstate *, int));
	void	(*bb_detach) __P((struct ddstate *));
	int	(*bb_clear) __P((struct ddstate *, int));
	int	(*bb_load) __P((struct ddstate *, enum loadhow, char *));
	int	(*bb_setaux) __P((struct ddstate *, char *));
	void	(*bb_scanlimit) __P((struct ddstate *, daddr_t *, daddr_t *));
	int	(*bb_addbfi) __P((struct ddstate *, int, int, int));
	int	(*bb_addsn) __P((struct ddstate *, daddr_t));
	int	(*bb_print) __P((struct ddstate *, enum printhow, int));
	int	(*bb_store) __P((struct ddstate *));
};

/* special pseudo error return from attach */
#define	NOT_AUTO	(-1)	/* might work, but auto-detect says not */

extern struct bbops *bbops[];
extern int nbbops;
