/*	BSDI token.h,v 1.1 1996/08/26 20:13:10 prb Exp */
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
 * Operations accepted by the token admin commands
 */

#define	TOKEN_DISABLE	0x1	/* disable user account		*/
#define	TOKEN_ENABLE	0x2	/* enable user account		*/
#define	TOKEN_INITUSER	0x4	/* add/init user account	*/
#define	TOKEN_RMUSER	0x8	/* remove user account		*/
#define	TOKEN_UNLOCK	0x10	/* force unlock db record	*/

/*
 * Flags for options to modify TOKEN_INITUSER
 */

#define	TOKEN_FORCEINIT	0x100	/* reinit existing account	*/
#define	TOKEN_GENSECRET	0x200	/* gen shared secret for token	*/

/*
 * Structure describing different token cards
 */
struct token_types {
	char	*name;		/* name of card */
	char	*proper;	/* proper name of card */
	char	*db;		/* path to database */
	char	map[6];		/* how A-F map to decimal */
	int	options;	/* varios available options */
	u_int	modes;		/* available modes */
	u_int	defmode;	/* default mode (if none specified) */	
};

struct token_types *tt;		/* what type we are running as now */

/*
 * Options
 */
#define	TOKEN_PHONE	0x0001	/* Allow phone number representation */
#define	TOKEN_HEXINIT	0x0002	/* Allow initialization in hex (and octal) */

/*
 * Function prototypes for commands involving intimate DES knowledge
 */

extern	void	tokenchallenge(char *, char *, int, char *);
extern	int	tokenverify(char *, char *, char *);
extern	int	tokenuserinit(int, char *, u_char *, u_int);
extern	int	token_init(char *);
extern	u_int	token_mode(char *);
extern	char *	token_getmode(u_int);
