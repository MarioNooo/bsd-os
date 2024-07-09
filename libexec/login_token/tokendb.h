/*	BSDI tokendb.h,v 1.1 1996/08/26 20:13:11 prb Exp */
/*-
//////////////////////////////////////////////////////////////////////////
//									//
//   Copyright (c) 1995 Migration Associates Corp. All Rights Reserved	//
//									//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MIGRATION ASSOCIATES	//
//	The copyright notice above does not evidence any   		//
//	actual or intended publication of such source code.		//
//									//
//	Use, duplication, or disclosure by the Government is		//
//	subject to restrictions as set forth in FAR 52.227-19,		//
//	and (for NASA) as supplemented in NASA FAR 18.52.227-19,	//
//	in subparagraph (c) (1) (ii) of Rights in Technical Data	//
//	and Computer Software clause at DFARS 252.227-7013, any		//
//	successor regulations or comparable regulations of other	//
//	Government agencies as appropriate.				//
//									//
//		Migration Associates Corporation			//
//			19935 Hamal Drive				//
//			Monument, CO 80132				//
//									//
//	A license is granted to Berkeley Software Design, Inc. by	//
//	Migration Associates Corporation to redistribute this software	//
//	under the terms and conditions of the software License		//
//	Agreement provided with this distribution.  The Berkeley	//
//	Software Design Inc. software License Agreement specifies the	//
//	terms and conditions for redistribution.			//
//									//
//	UNDER U.S. LAW, THIS SOFTWARE MAY REQUIRE A LICENSE FROM	//
//	THE U.S. COMMERCE DEPARTMENT TO BE EXPORTED.  IT IS YOUR	//
//	REQUIREMENT TO OBTAIN THIS LICENSE PRIOR TO EXPORTATION.	//
//									//
//////////////////////////////////////////////////////////////////////////
*/

/*
 * Structure defining a record for a user.  All fields 
 * stored in ascii to facilitate backup/reconstruction.
 * A null byte is required after the share secret field.
 */

typedef	struct	{
	char	uname[L_cuserid];	/* user login name	*/
	char	secret[16];		/* token shared secret	*/
	unsigned flags;			/* record flags		*/
	unsigned mode;			/* token mode flags	*/
	time_t	lock_time;		/* time of record lock	*/
	u_char	rim[8];			/* reduced input mode 	*/
	char	reserved_char1[8];
	char	reserved_char2[80];
} TOKENDB_Rec;

/*
 * Record flag values
 */

#define	TOKEN_LOCKED	0x1		/* record locked for updating	*/
#define	TOKEN_ENABLED	0x2		/* user login method enabled	*/
#define	TOKEN_LOGIN	0x4		/* login in progress lock	*/
#define	TOKEN_USEMODES	0x8		/* use the mode field		*/

#define	TOKEN_DECMODE	0x1		/* allow decimal results */
#define	TOKEN_HEXMODE	0x2		/* allow hex results */
#define	TOKEN_PHONEMODE	0x4		/* allow phone book results */
#define	TOKEN_RIM	0x8		/* reduced imput mode */

/*
 * Function prototypes for routines which manipulate the 
 * database for the token.  These routines have no knowledge
 * of DES or encryption.  However, they will manipulate the
 * flags field of the database record with complete abandon.
 */

extern	int	tokendb_delrec(char *);
extern	int	tokendb_getrec(char *, TOKENDB_Rec *);
extern	int	tokendb_putrec(char *, TOKENDB_Rec *);
extern	int	tokendb_firstrec(int, TOKENDB_Rec *);
extern	int	tokendb_nextrec(int, TOKENDB_Rec *);
extern	int	tokendb_lockrec(char *, TOKENDB_Rec *, unsigned);
