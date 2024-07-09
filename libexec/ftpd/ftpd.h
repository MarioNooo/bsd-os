/*-
 * Copyright (c) 1996,1998 Berkeley Software Design, Inc. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this notice is retained,
 * the conditions in the following notices are met, and terms applying
 * to contributors in the following notices also apply to Berkeley
 * Software Design, Inc.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *      Berkeley Software Design, Inc.
 * 4. Neither the name of the Berkeley Software Design, Inc. nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      BSDI ftpd.h,v 1.3 1998/02/04 19:14:26 prb Exp
 */
#ifndef INET6
#define INET6 1
#endif
#ifndef IPSEC
#define IPSEC 1
#endif

#define	CMD	0	/* beginning of command */
#define	ARGS	1	/* expect miscellaneous arguments */
#define	STR1	2	/* expect SP followed by STRING */
#define	STR2	3	/* expect STRING */
#define	OSTR	4	/* optional SP then STRING */
#define	ZSTR1	5	/* SP then optional STRING */
#define	ZSTR2	6	/* optional STRING after SP */
#define	SITECMD	7	/* SITE command */
#define	NSTR	8	/* Number followed by a string */

struct tab {
        char    *name;
        short   token;
        short   state;
        char    implemented;    /* 1 if command is implemented */
        char    guestokay;      /* 1 if guest can use it */
        char    *help;
};

#ifdef	REG_EXTENDED
struct regexps {
	struct	regexps *next;
	char	*pattern;
	regex_t	preg;
};
#else
struct regexps;
#endif

struct ftpdir {
	struct	ftpdir *next;
	char	*path;		/* Pathname to incoming file */
	dev_t	dev;		/* device of this directory */
	ino_t	ino;		/* inode number of this directory */
	uid_t	uid;		/* uid we store files as */
	gid_t	gid;		/* gid we store files as */
	mode_t	mode;		/* mode we store files as */
	mode_t	dmode;		/* mode we store directories as */
};

struct pidlist {
	struct	pidlist *next;
	pid_t	pid;
	struct	ftphost	*host;
};

#if INET6
#define sockaddr_generic sockaddr_in6
#else /* INET6 */
#define sockaddr_generic sockaddr_in
#endif /* INET6 */

#define SA(sa)	 ((struct sockaddr *)sa)
#define SIN(sa)	 ((struct sockaddr_in *)sa)
#define SIN6(sa) ((struct sockaddr_in6 *)sa)
	

struct ftphost {
	struct ftphost	*next;
	struct sockaddr_generic	addr;
	struct ftpdir	*ftpdirs;
	int		sessions;

	char		*chrootlist;
	char		*banlist;
	char		*permitlist;
	char		*groupfile;
        char            *anondir;
        char            *anonuser;
        char            *hostname;
        char            *loginmsg;
        char            *statfile;
        char            *welcome;
	char		*logfmt;
	char		*tmplogfmt;
	char		*msgfile;

	struct regexps	*pathfilter;

	u_long		maxusers;
	u_long		flags;
	u_long		maxtimeout;
	u_long		timeout;
	u_long		umask;
	u_char		*sitecmds;
	u_char		cmds[4];
};

#define	ANON_ONLY	0x0001
#define	BUILTIN_LS	0x0002
#define	DEBUG		0x0004
#define	LOGGING		0x0008
#define	ELOGGING	0x0010
#define	PROXY_OK	0x0020
#define	STATS		0x0040
#define	HIGHPORTS	0x0080
#define	RESTRICTEDPORTS	0x0100
#define	VIRTUALONLY	0x0200
#define	ALLOW_ANON	0x0400
#define	VIRTUAL		0x0800
#define	KEEPALIVE	0x1000
#define	USERFC931	0x2000
#define	PARSED		0x4000

/*
 * We maintain a guest version of the flags
 * GUESTSHIFT bits up in the flags word
 */
#define	GUESTSHIFT	16
					

extern struct ftphost *thishost, *defhost;
extern struct ftpdir *ftpdirs;

#define	FL(x)	(thishost->flags & ((x) << (guest ? GUESTSHIFT : 0)))
#define	OP(x)	(thishost->x)

#if !defined(CMASK) || CMASK == 0
#undef CMASK
#define CMASK 027
#endif

extern struct tab cmdtab[];
extern struct tab sitetab[];
extern int ncmdtab;
extern int nsitetab;
extern struct pidlist *pidlist;

extern int show_error;
extern int guest;
