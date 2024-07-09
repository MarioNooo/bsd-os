/*
 * Copyright (c) 1985, 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ftpcmd.y	8.3 (Berkeley) 4/6/94
 */

/*
 * Grammar for FTP commands.
 * See RFC 959.
 */

%{

#ifndef lint
#if 0
static char sccsid[] = "@(#)ftpcmd.y	8.3 (Berkeley) 4/6/94";
#endif
static const char rcsid[] =
	"ftpcmd.y,v 1.13 2003/03/06 20:39:10 dab Exp";
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/ftp.h>

#include <ctype.h>
#include <errno.h>
#include <glob.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#ifdef	FREEBSD
#include <libutil.h>
#endif

#include "extern.h"
#include "ftpd.h"

int	epsvall = 0;
extern	struct sockaddr_generic data_dest, his_addr, ctrl_addr;
extern	int logged_in;
extern	struct passwd *pw;
extern	int guest;
extern	int type;
extern	int form;
extern  int pdata;
extern	char remotehost[];
extern	char proctitle[];
extern	int usedefault;
extern  int transflag;
extern  char tmpline[];
extern	int reply_code;

off_t	restart_point;

static	int cmd_type;
static	int cmd_form;
static	int cmd_bytesz;
char	cbuf[512];
char	*fromname;

int sporttosa __P((struct sockaddr *, char *, struct sockaddr *));
int lporttosa __P((struct sockaddr *, char *, struct sockaddr *));
int eporttosa __P((struct sockaddr *, char *, struct sockaddr *, int));


%}

%union {
	int	i;
	char   *s;
}

%token
	A	B	C	E	F	I
	L	N	P	R	S	T

	SP	CRLF	COMMA

	USER	PASS	ACCT	REIN	QUIT	PORT
	PASV	TYPE	STRU	MODE	RETR	STOR
	APPE	MLFL	MAIL	MSND	MSOM	MSAM
	MRSQ	MRCP	ALLO	REST	RNFR	RNTO
	ABOR	DELE	CWD	LIST	NLST	SITE
	STAT	HELP	NOOP	MKD	RMD	PWD
	CDUP	STOU	SMNT	SYST	SIZE	MDTM
	LPSV	LPRT	SPASV	SPORT	EPSV	EPRT

	UMASK	IDLE	CHMOD	GROUP	GPASS

	LEXERR

%token	<s> STRING
%token	<i> NUMBER

%type	<i> check_login octal_number byte_size
%type	<i> struct_code mode_code type_code form_code
%type	<s> pathstring pathname password username
%type	<i> host_port

%start	cmd_list

%%

cmd_list
	: /* empty */
	| cmd_list cmd
		{
			fromname = (char *) 0;
			restart_point = (off_t) 0;
		}
	| cmd_list rcmd
	;

cmd
	: USER SP username CRLF
		{
			user($3);
			free($3);
		}
	| PASS SP password CRLF
		{
			pass($3);
			free($3);
		}
	| PORT check_login SP host_port CRLF
		{
			if (!$2)
				; /*not logged in*/
			else if (epsvall) {
				reply(501,
				    "PORT not allowed after an EPSV ALL.");
			} else if (port_ok("PORT")) {
				usedefault = 0;
				if (pdata >= 0) {
					(void) close(pdata);
					pdata = -1;
				}
				reply(200, "PORT command successful.");
			}
		}
	| EPRT check_login SP pathname CRLF
		{
			if (!$2)
				; /* not logged in */
			else if (epsvall) {
				reply(501,
				    "EPRT not allowed after an EPSV ALL.");
			} else {

				switch(eporttosa(SA(&data_dest), $4,
				    SA(&his_addr), 0)) {
				case 0:
					if (port_ok("EPRT")) {
						usedefault = 0;
						if (pdata >= 0) {
							(void) close(pdata);
							pdata = -1;
						}
						reply(200,
						    "EPRT command successful.");
					}
					break;
				case -2:
					reply(522, "Network protocol not "
					    "supported, use"
#if INET6
					    " (1,2)"	/* IP4 & IP6*/
#else /* INET6 */
					    " (1)"	/* IP4*/
#endif /* INET6 */
					    );
					break;
				case -3:
					reply(501, "Address invalid for"
					    " your control connection");
					syslog(LOG_WARNING,
					    "refused bogus %s from %s",
					    cbuf, remotehost);
					break;
				default:
					reply(501,
					    "Address syntax invalid");
					break;
				}
			}
		}

	| SPORT check_login SP pathname CRLF
		{
			int ret;

			if (!$2)
				; /* not logged in */
			else if (epsvall) {
				reply(501,
				    "SPORT not allowed after an EPSV ALL.");
			} else {

				ret = sporttosa(SA(&data_dest), $4,
				    SA(&his_addr));
				switch(ret) {
				case 0:
					if (!port_ok("SPORT"))
						break;
					usedefault = 0;
					if (pdata >= 0) {
						(void) close(pdata);
						pdata = -1;
					}
					reply(200, "SPORT command successful.");
					break;
				case -3:
					reply(501, "Address invalid for your "
					    "control connection");
					break;
				default:
					reply(501,
					    "Address syntax invalid (ret = %d)",
					    ret);
					break;
				}
			}
		}
	| LPRT check_login SP pathname CRLF
		{
			if (!$2)
				; /* not logged in */
			else if (epsvall) {
				reply(501,
				    "LPRT not allowed after an EPSV ALL.");
			} else {
				switch(lporttosa(SA(&data_dest), $4,
				    SA(&his_addr))) {
				case 0:
					if (!port_ok("LPRT"))
						break;
					usedefault = 0;
					if (pdata >= 0) {
						(void) close(pdata);
						pdata = -1;
					}
					reply(200, "LPRT command successful.");
					break;
				case -2:
					reply(501, "Address invalid for a"
					    " af#%d control connection",
					    SA(&ctrl_addr)->sa_family);
					break;
				case -3:
					reply(501, "Address invalid for your "
					    "control connection");
					break;
				default:
					reply(501, "Address syntax invalid");
					break;
				}
			}
		}
	| PASV check_login CRLF
		{
			if (!$2)
				;
			else if (epsvall)
				reply(501,
				    "PASV not allowed after an EPSV ALL.");
			else
				passive(2, SA(&his_addr)->sa_family);
		}
        | EPSV check_login CRLF
                {
			if ($2)
				passive(1, SA(&his_addr)->sa_family);
                }
        | EPSV check_login SP STRING CRLF
		{
			if (!$2)
				;
			else if (strcasecmp($4, "ALL") == 0) {
				epsvall = 1;
				reply(200, "EPSV ALL command successful.");
			} else {
				char *c;
				int af;

				af = strtoul($4, &c, 0);
				if (*c)
					reply(501, "'EPSV %s': command not "
					    "understood.", $4);
				else switch(af) {
				case 1:
					passive(1, AF_INET);
					break;
#if INET6
				case 2:
					passive(1, AF_INET6);
					break;
#endif /* INET6 */
				default:
					reply(522, "Network protocol not "
					    "supported, use"
#if INET6
					    " (1,2)"	/* IP4,IP6 */
#else /* INET6 */
					    " (1)"	/* IP4 */
#endif /* INET6 */
					    );
				}
			}
                }
	| SPASV check_login CRLF
		{
			if (!$2)
				;
			else if (epsvall) 
				reply(501,
				    "SPASV not allowed after an EPSV ALL.");
			else    
				passive(3, SA(&his_addr)->sa_family);
		}
	| LPSV check_login CRLF
		{
			if (!$2)
				;
			else if (epsvall) 
				reply(501,
				    "LPASV not allowed after an EPSV ALL.");
			else    
				passive(4, SA(&his_addr)->sa_family);
		}
	| TYPE SP type_code CRLF
		{
			switch (cmd_type) {

			case TYPE_A:
				if (cmd_form == FORM_N) {
					reply(200, "Type set to A.");
					type = cmd_type;
					form = cmd_form;
				} else
					reply(504, "Form must be N.");
				break;

			case TYPE_E:
				reply(504, "Type E not implemented.");
				break;

			case TYPE_I:
				reply(200, "Type set to I.");
				type = cmd_type;
				break;

			case TYPE_L:
#if NBBY == 8
				if (cmd_bytesz == 8) {
					reply(200,
					    "Type set to L (byte size 8).");
					type = cmd_type;
				} else
					reply(504, "Byte size must be 8.");
#else /* NBBY == 8 */
				UNIMPLEMENTED for NBBY != 8
#endif /* NBBY == 8 */
			}
		}
	| STRU SP struct_code CRLF
		{
			switch ($3) {

			case STRU_F:
				reply(200, "STRU F ok.");
				break;

			default:
				reply(504, "Unimplemented STRU type.");
			}
		}
	| MODE SP mode_code CRLF
		{
			switch ($3) {

			case MODE_S:
				reply(200, "MODE S ok.");
				break;

			default:
				reply(502, "Unimplemented MODE type.");
			}
		}
	| ALLO SP NUMBER CRLF
		{
			reply(202, "ALLO command ignored.");
		}
	| ALLO SP NUMBER SP R SP NUMBER CRLF
		{
			reply(202, "ALLO command ignored.");
		}
	| RETR check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				retrieve((char *) 0, $4);
			if ($4 != NULL)
				free($4);
		}
	| STOR check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				store($4, "w", 0);
			if ($4 != NULL)
				free($4);
		}
	| APPE check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				store($4, "a", 0);
			if ($4 != NULL)
				free($4);
		}
	| NLST check_login CRLF
		{
			if ($2)
				send_file_list(".");
		}
	| NLST check_login SP STRING CRLF
		{
			if ($2 && $4 != NULL)
				send_file_list($4);
			if ($4 != NULL)
				free($4);
		}
	| LIST check_login CRLF
		{
			if ($2)
				retrieve("/bin/ls -lgA", "");
		}
	| LIST check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				retrieve("/bin/ls -lgA %s", $4);
			if ($4 != NULL)
				free($4);
		}
	| STAT check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				statfilecmd($4);
			if ($4 != NULL)
				free($4);
		}
	| STAT CRLF
		{
			statcmd();
		}
	| DELE check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				delete($4);
			if ($4 != NULL)
				free($4);
		}
	| RNTO check_login SP pathname CRLF
		{
			if ($2) {
				if (fromname) {
					renamecmd(fromname, $4);
					free(fromname);
					fromname = (char *) 0;
				} else {
					reply(503, "Bad sequence of commands.");
				}
			}
			free($4);
		}
	| ABOR CRLF
		{
			reply(225, "ABOR command successful.");
		}
	| CWD check_login CRLF
		{
			if ($2)
				cwd(pw->pw_dir);
		}
	| CWD check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				cwd($4);
			if ($4 != NULL)
				free($4);
		}
	| HELP CRLF
		{
			help(cmdtab, (char *) 0);
		}
	| HELP SP STRING CRLF
		{
			char *cp = $3;

			if (strncasecmp(cp, "SITE", 4) == 0) {
				cp = $3 + 4;
				if (*cp == ' ')
					cp++;
				if (*cp)
					help(sitetab, cp);
				else
					help(sitetab, (char *) 0);
			} else
				help(cmdtab, $3);
		}
	| NOOP CRLF
		{
			reply(200, "NOOP command successful.");
		}
	| MKD check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				makedir($4);
			if ($4 != NULL)
				free($4);
		}
	| RMD check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				removedir($4);
			if ($4 != NULL)
				free($4);
		}
	| PWD check_login CRLF
		{
			if ($2)
				pwd();
		}
	| CDUP check_login CRLF
		{
			if ($2)
				cwd("..");
		}
	| SITE SP HELP CRLF
		{
			help(sitetab, (char *) 0);
		}
	| SITE SP HELP SP STRING CRLF
		{
			help(sitetab, $5);
		}
	| SITE SP UMASK check_login CRLF
		{
			int oldmask;

			if ($4) {
				oldmask = umask(0);
				(void) umask(oldmask);
				reply(200, "Current UMASK is %03o", oldmask);
			}
		}
	| SITE SP UMASK check_login SP octal_number CRLF
		{
			int oldmask;

			if ($4) {
				if (($6 == -1) || ($6 > 0777)) {
					reply(501, "Bad UMASK value");
				} else {
					oldmask = umask($6);
					reply(200,
					    "UMASK set to %03o (was %03o)",
					    $6, oldmask);
				}
			}
		}
	| SITE SP CHMOD check_login SP octal_number SP pathname CRLF
		{
			if ($4 && ($8 != NULL)) {
				if ($6 > 0777)
					reply(501,
				"CHMOD: Mode value must be between 0 and 0777");
				else if (chmod($8, $6) < 0)
					perror_reply(550, $8);
				else
					reply(200, "CHMOD command successful.");
			}
			if ($8 != NULL)
				free($8);
		}
	| SITE SP IDLE CRLF
		{
			reply(200,
			    "Current IDLE time limit is %d seconds; max %d",
				OP(timeout), OP(maxtimeout));
		}
	| SITE SP IDLE SP NUMBER CRLF
		{
			if ($5 < 30 || $5 > OP(maxtimeout)) {
				reply(501,
			"Maximum IDLE time must be between 30 and %d seconds",
				    OP(maxtimeout));
			} else {
				OP(timeout) = $5;
				(void) alarm((unsigned) OP(timeout));
				reply(200,
				    "Maximum IDLE time set to %d seconds",
				    OP(timeout));
			}
		}
	| SITE SP GROUP check_login SP username CRLF
        	{
			if ($4 && $6 != NULL)
				group($6);
			if ($6 != NULL)
            			free($6);
		}
	| SITE SP GPASS check_login SP password CRLF
        	{
			if ($4 && $6 != NULL)
				gpass($6);
			if ($6 != NULL)
            			free($6);
        }
	| STOU check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				store($4, "w", 1);
			if ($4 != NULL)
				free($4);
		}
	| SYST CRLF
		{
#ifdef unix
#ifdef BSD
			reply(215, "UNIX Type: L%d Version: BSD-%d",
#ifdef _BSDI_VERSION
				NBBY, _BSDI_VERSION);
#else /* !_BSDI_VERSION */
				NBBY, BSD);
#endif /* _BSDI_VERSION */
#else /* BSD*/
			reply(215, "UNIX Type: L%d", NBBY);
#endif /* BSD */
#else /* unix */
			reply(215, "UNKNOWN Type: L%d", NBBY);
#endif /* unix */
		}

		/*
		 * SIZE is not in RFC959, but Postel has blessed it and
		 * it will be in the updated RFC.
		 *
		 * Return size of file in a format suitable for
		 * using with RESTART (we just count bytes).
		 */
	| SIZE check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				sizecmd($4);
			if ($4 != NULL)
				free($4);
		}

		/*
		 * MDTM is not in RFC959, but Postel has blessed it and
		 * it will be in the updated RFC.
		 *
		 * Return modification time of file as an ISO 3307
		 * style time. E.g. YYYYMMDDHHMMSS or YYYYMMDDHHMMSS.xxx
		 * where xxx is the fractional second (of any precision,
		 * not necessarily 3 digits)
		 */
	| MDTM check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL) {
				struct stat stbuf;
				if (stat($4, &stbuf) < 0)
					reply(550, "%s: %s",
					    $4, strerror(errno));
				else if (!S_ISREG(stbuf.st_mode)) {
					reply(550, "%s: not a plain file.", $4);
				} else {
					struct tm *t;
					t = gmtime(&stbuf.st_mtime);
					reply(213,
					    "%04d%02d%02d%02d%02d%02d",
					    1900 + t->tm_year,
					    t->tm_mon+1, t->tm_mday,
					    t->tm_hour, t->tm_min, t->tm_sec);
				}
			}
			if ($4 != NULL)
				free($4);
		}
	| QUIT CRLF
		{
			reply(221, "Goodbye.");
			dologout(0);
		}
	| error CRLF
		{
			yyerrok;
		}
	;
rcmd
	: RNFR check_login SP pathname CRLF
		{
			char *renamefrom();

			restart_point = (off_t) 0;
			if ($2 && $4) {
				fromname = renamefrom($4);
				if (fromname == (char *) 0 && $4) {
					free($4);
				}
			}
		}
	| REST SP byte_size CRLF
		{
			fromname = (char *) 0;
			restart_point = $3;	/* XXX $3 is only "int" */
			reply(350, "Restarting at %qd. %s", restart_point,
			    "Send STORE or RETRIEVE to initiate transfer.");
		}
	;

username
	: STRING
	;

password
	: /* empty */
		{
			$$ = (char *)calloc(1, sizeof(char));
		}
	| STRING
	;

byte_size
	: NUMBER
	;

host_port
	: NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER COMMA NUMBER
		{
#if INET6
			char *p;

			$$ = 0;
			if (logged_in) {
				memcpy(&data_dest, &his_addr,
				    SA(&ctrl_addr)->sa_len);

				switch(SA(&his_addr)->sa_family) {
				case AF_INET:
					p = (char *)&SIN(&data_dest)->sin_addr;
					p[0] = $1; p[1] = $3;
					p[2] = $5; p[3] = $7;

					p = (char *)&SIN(&data_dest)->sin_port;
					p[0] = $9; p[1] = $11;

					if (!memcmp(&SIN(&data_dest)->sin_addr,
					    &SIN(&his_addr)->sin_addr,
					    sizeof(struct in_addr)) &&
					    (SIN(&data_dest)->sin_port > 1023))
						$$ = 1;
					break;
				case AF_INET6:
					if (IN6_IS_ADDR_V4MAPPED(
					    &SIN6(&his_addr)->sin6_addr)) {
						p = (char *)
						   &SIN6(&data_dest)->sin6_addr;
						p[12] = $1; p[13] = $3;
						p[14] = $5; p[15] = $7;

						p = (char *)
						    &SIN6(&data_dest)->sin6_port;
						p[0] = $9; p[1] = $11;

						if (!memcmp(
						    &SIN6(&data_dest)->sin6_addr,
						    &SIN6(&his_addr)->sin6_addr,
						    sizeof(struct in6_addr)) &&
						    (SIN6(&data_dest)->sin6_port >
						    1023))
							$$ = 1;
						break;
					}
				default:
					reply(501, "Address can't be used "
					    "with a af#%d control connection",
					    SA(&ctrl_addr)->sa_family);
					break;
				}
			}
#else /* INET6 */
			char *a, *p;

			SIN(&data_dest)->sin_len = sizeof(struct sockaddr_in);
			SIN(&data_dest)->sin_family = AF_INET;
			p = (char *)&SIN(&data_dest)->sin_port;
			p[0] = $9; p[1] = $11;
			a = (char *)&SIN(&data_dest)->sin_addr;
			a[0] = $1; a[1] = $3; a[2] = $5; a[3] = $7;
#endif /* INET6 */
		}
	;

form_code
	: N
		{
			$$ = FORM_N;
		}
	| T
		{
			$$ = FORM_T;
		}
	| C
		{
			$$ = FORM_C;
		}
	;

type_code
	: A
		{
			cmd_type = TYPE_A;
			cmd_form = FORM_N;
		}
	| A SP form_code
		{
			cmd_type = TYPE_A;
			cmd_form = $3;
		}
	| E
		{
			cmd_type = TYPE_E;
			cmd_form = FORM_N;
		}
	| E SP form_code
		{
			cmd_type = TYPE_E;
			cmd_form = $3;
		}
	| I
		{
			cmd_type = TYPE_I;
		}
	| L
		{
			cmd_type = TYPE_L;
			cmd_bytesz = NBBY;
		}
	| L SP byte_size
		{
			cmd_type = TYPE_L;
			cmd_bytesz = $3;
		}
		/* this is for a bug in the BBN ftp */
	| L byte_size
		{
			cmd_type = TYPE_L;
			cmd_bytesz = $2;
		}
	;

struct_code
	: F
		{
			$$ = STRU_F;
		}
	| R
		{
			$$ = STRU_R;
		}
	| P
		{
			$$ = STRU_P;
		}
	;

mode_code
	: S
		{
			$$ = MODE_S;
		}
	| B
		{
			$$ = MODE_B;
		}
	| C
		{
			$$ = MODE_C;
		}
	;

pathname
	: pathstring
		{
			/*
			 * Problem: this production is used for all pathname
			 * processing, but only gives a 550 error reply.
			 * This is a valid reply in some cases but not in others.
			 */
			if (logged_in && $1 && *$1 == '~') {
				glob_t gl;
				int flags =
				 GLOB_BRACE|GLOB_NOCHECK|GLOB_QUOTE|GLOB_TILDE;

				memset(&gl, 0, sizeof(gl));
				if (glob($1, flags, NULL, &gl) ||
				    gl.gl_pathc == 0) {
					reply(550, "not found");
					$$ = NULL;
				} else {
					$$ = strdup(gl.gl_pathv[0]);
				}
				globfree(&gl);
				free($1);
			} else
				$$ = $1;
		}
	;

pathstring
	: STRING
	;

octal_number
	: NUMBER
		{
			int ret, dec, multby, digit;

			/*
			 * Convert a number that was read as decimal number
			 * to what it would be if it had been read as octal.
			 */
			dec = $1;
			multby = 1;
			ret = 0;
			while (dec) {
				digit = dec%10;
				if (digit > 7) {
					ret = -1;
					break;
				}
				ret += digit * multby;
				multby *= 8;
				dec /= 10;
			}
			$$ = ret;
		}
	;


check_login
	: /* empty */
		{
			if (logged_in)
				$$ = 1;
			else {
				reply(530, "Please login with USER and PASS.");
				$$ = 0;
			}
		}
	;

%%

extern jmp_buf errcatch;

struct tab cmdtab[] = {		/* In order defined in RFC 765 */
	{ "USER", USER, STR1, 1, 1,	"<sp> username" },
	{ "PASS", PASS, ZSTR1, 1, 1,	"<sp> password" },
	{ "ACCT", ACCT, STR1, 0, 0,	"(specify account)" },
	{ "SMNT", SMNT, ARGS, 0, 0,	"(structure mount)" },
	{ "REIN", REIN, ARGS, 0, 0,	"(reinitialize server state)" },
	{ "QUIT", QUIT, ARGS, 1, 1,	"(terminate service)", },
	{ "PORT", PORT, ARGS, 1, 1,	"<sp> b0, 0, b1, b2, b3, b4" },
	{ "PASV", PASV, ARGS, 1, 1,	"(set server in passive mode)" },
        { "EPRT", EPRT, STR1, 1, 1,	"<sp> <d> af <d> host <d> service <d>"},
        { "EPSV", EPSV, OSTR, 1, 1,	"(set server in passive mode)" },
	{ "SPORT", SPORT, STR1, 1, 1,	"<sp> pl, p1, ..." },
	{ "SPASV", SPASV, ARGS, 1, 1,	"(set server in passive mode)" },
	{ "LPRT", LPRT, STR1, 1, 1,	"<sp> af, al, a1, ..., pl, p1, ..." },
	{ "LPSV", LPSV, ARGS, 1, 1,	"(set server in passive mode)" },
	{ "TYPE", TYPE, ARGS, 1, 1,	"<sp> [ A | E | I | L ]" },
	{ "STRU", STRU, ARGS, 1, 1,	"(specify file structure)" },
	{ "MODE", MODE, ARGS, 1, 1,	"(specify transfer mode)" },
	{ "RETR", RETR, STR1, 1, 1,	"<sp> file-name" },
	{ "STOR", STOR, STR1, 1, 1,	"<sp> file-name" },
	{ "APPE", APPE, STR1, 1, 0,	"<sp> file-name" },
	{ "MLFL", MLFL, OSTR, 0, 0,	"(mail file)" },
	{ "MAIL", MAIL, OSTR, 0, 0,	"(mail to user)" },
	{ "MSND", MSND, OSTR, 0, 0,	"(mail send to terminal)" },
	{ "MSOM", MSOM, OSTR, 0, 0,	"(mail send to terminal or mailbox)" },
	{ "MSAM", MSAM, OSTR, 0, 0,	"(mail send to terminal and mailbox)" },
	{ "MRSQ", MRSQ, OSTR, 0, 0,	"(mail recipient scheme question)" },
	{ "MRCP", MRCP, STR1, 0, 0,	"(mail recipient)" },
	{ "ALLO", ALLO, ARGS, 1, 1,	"allocate storage (vacuously)" },
	{ "REST", REST, ARGS, 1, 1,	"<sp> offset (restart command)" },
	{ "RNFR", RNFR, STR1, 1, 0,	"<sp> file-name" },
	{ "RNTO", RNTO, STR1, 1, 0,	"<sp> file-name" },
	{ "ABOR", ABOR, ARGS, 1, 1,	"(abort operation)" },
	{ "DELE", DELE, STR1, 1, 0,	"<sp> file-name" },
	{ "CWD",  CWD,  OSTR, 1, 1,	"[ <sp> directory-name ]" },
	{ "XCWD", CWD,	OSTR, 1, 1,	"[ <sp> directory-name ]" },
	{ "LIST", LIST, OSTR, 1, 1,	"[ <sp> path-name ]" },
	{ "NLST", NLST, OSTR, 1, 1,	"[ <sp> path-name ]" },
	{ "SITE", SITE, SITECMD, 1, 1,	"site-cmd [ <sp> arguments ]" },
	{ "SYST", SYST, ARGS, 1, 1,	"(get type of operating system)" },
	{ "STAT", STAT, OSTR, 1, 1,	"[ <sp> path-name ]" },
	{ "HELP", HELP, OSTR, 1, 1,	"[ <sp> <string> ]" },
	{ "NOOP", NOOP, ARGS, 1, 1,	"" },
	{ "MKD",  MKD,  STR1, 1, 0,	"<sp> path-name" },
	{ "XMKD", MKD,  STR1, 1, 0,	"<sp> path-name" },
	{ "RMD",  RMD,  STR1, 1, 0,	"<sp> path-name" },
	{ "XRMD", RMD,  STR1, 1, 0,	"<sp> path-name" },
	{ "PWD",  PWD,  ARGS, 1, 1,	"(return current directory)" },
	{ "XPWD", PWD,  ARGS, 1, 1,	"(return current directory)" },
	{ "CDUP", CDUP, ARGS, 1, 1,	"(change to parent directory)" },
	{ "XCUP", CDUP, ARGS, 1, 1,	"(change to parent directory)" },
	{ "STOU", STOU, STR1, 1, 1,	"<sp> file-name" },
	{ "SIZE", SIZE, OSTR, 1, 1,	"<sp> path-name" },
	{ "MDTM", MDTM, OSTR, 1, 1,	"<sp> path-name" },
	{ NULL,   0,    0,    0, 0,	0 }
};

struct tab sitetab[] = {
	{ "UMASK", UMASK, ARGS, 1, 0,	"[ <sp> umask ]" },
	{ "IDLE", IDLE, ARGS, 1, 0,	"[ <sp> maximum-idle-time ]" },
	{ "CHMOD", CHMOD, NSTR, 1, 0,	"<sp> mode <sp> file-name" },
	{ "HELP", HELP, OSTR, 1, 1,	"[ <sp> <string> ]" },
	{ "GROUP", GROUP, STR1, 1, 1,	"<sp> access-group" },
	{ "GPASS", GPASS, STR1, 1, 1,	"<sp> access-password" },
	{ NULL,   0,    0,    0, 0,	0 }
};

int ncmdtab = (sizeof(cmdtab)/sizeof(cmdtab[0]));
int nsitetab = (sizeof(sitetab)/sizeof(sitetab[0]));

static char	*copy __P((char *));
static void	 help __P((struct tab *, char *));
static struct tab *
		 lookup __P((struct tab *, char *));
static void	 sizecmd __P((char *));
static void	 toolong __P((int));
static int	 yylex __P((void));
static int	 port_ok __P((char *));

static struct tab *
lookup(p, cmd)
	struct tab *p;
	char *cmd;
{

	for (; p->name != NULL; p++)
		if (strcmp(cmd, p->name) == 0)
			return (p);
	return (0);
}

#include <arpa/telnet.h>

/*
 * getline - a hacked up version of fgets to ignore TELNET escape codes.
 */
char *
getline(s, n, iop)
	char *s;
	int n;
	FILE *iop;
{
	int c;
	register char *cs;

	cs = s;
/* tmpline may contain saved command from urgent mode interruption */
	for (c = 0; tmpline[c] != '\0' && --n > 0; ++c) {
		*cs++ = tmpline[c];
		if (tmpline[c] == '\n') {
			*cs++ = '\0';
			if (FL(DEBUG))
				syslog(LOG_DEBUG, "command: %s", s);
			tmpline[0] = '\0';
			return(s);
		}
		if (c == 0)
			tmpline[0] = '\0';
	}
	while ((c = getc(iop)) != EOF) {
		c &= 0377;
		if (c == IAC) {
		    if ((c = getc(iop)) != EOF) {
			c &= 0377;
			switch (c) {
			case WILL:
				c = getc(iop);
				printf("%c%c%c", IAC, DONT, 0377&c);
				(void) fflush(stdout);
				continue;
			case DO:
				c = getc(iop);
				printf("%c%c%c", IAC, WONT, 0377&c);
				(void) fflush(stdout);
				continue;
			case WONT:
			case DONT:
				/*
				 * Don't reply to these since a
				 * simple minded client can go
				 * into a loop (such as ours)
				 */
			case IAC:
				break;
			default:
				continue;	/* ignore command */
			}
		    }
		}
		*cs++ = c;
		if (--n <= 0 || c == '\n')
			break;
	}
	if (c == EOF && cs == s)
		return (NULL);
	*cs++ = '\0';
	if (FL(DEBUG)) {
		if (!guest && strncasecmp("pass ", s, 5) == 0) {
			/* Don't syslog passwords */
			syslog(LOG_DEBUG, "command: %.5s ???", s);
		} else {
			register char *cp;
			register int len;

			/* Don't syslog trailing CR-LF */
			len = strlen(s);
			cp = s + len - 1;
			while (cp >= s && (*cp == '\n' || *cp == '\r')) {
				--cp;
				--len;
			}
			syslog(LOG_DEBUG, "command: %.*s", len, s);
		}
	}
	return (s);
}

static void
toolong(signo)
	int signo;
{

	reply(421,
	    "Timeout (%d seconds): closing control connection.", OP(timeout));
	if (FL(LOGGING))
		syslog(LOG_INFO, "User %s timed out after %d seconds",
		    (pw ? pw -> pw_name : "unknown"), OP(timeout));
	dologout(1);
}

static int
yylex()
{
	static int cpos, state;
	char *cp, *cp2;
	struct tab *p;
	int n;
	char c;

	for (;;) {
		switch (state) {

		case CMD:
			(void) signal(SIGALRM, toolong);
			(void) alarm((unsigned) OP(timeout));
			reply_code = 0;	/* New line, no code sent yet */
			if (getline(cbuf, sizeof(cbuf)-1, stdin) == NULL) {
				reply(221, "You could at least say goodbye.");
				dologout(0);
			}
			(void) alarm(0);
#ifdef SETPROCTITLE
			if (strncasecmp(cbuf, "PASS", 4) == 0)
				cp = &cbuf[4];
			else if (strncasecmp(cbuf, "SITE GPASS", 10) == 0)
				cp = &cbuf[10];
			else if ((cp = strpbrk(cbuf, "\r\n")) == NULL)
				cp = &c;
			c = *cp;
			*cp = '\0';
			setproctitle("%s: %s", proctitle, cbuf);
			*cp = c;
#endif /* SETPROCTITLE */
			if ((cp = strchr(cbuf, '\r'))) {
				*cp++ = '\n';
				*cp = '\0';
			}
			if ((cp = strpbrk(cbuf, " \n")))
				cpos = cp - cbuf;
			if (cpos == 0)
				cpos = 4;
			c = cbuf[cpos];
			cbuf[cpos] = '\0';
			upper(cbuf);
			p = lookup(cmdtab, cbuf);
			cbuf[cpos] = c;
			if (p != 0) {
				if (p->implemented != 1 ||
				    (guest && p->guestokay == 0)) {
					nack(p->name);
					longjmp(errcatch,0);
					/* NOTREACHED */
				}
				state = p->state;
				yylval.s = p->name;
				return (p->token);
			}
			break;

		case SITECMD:
			if (cbuf[cpos] == ' ') {
				cpos++;
				return (SP);
			}
			cp = &cbuf[cpos];
			if ((cp2 = strpbrk(cp, " \n")))
				cpos = cp2 - cbuf;
			c = cbuf[cpos];
			cbuf[cpos] = '\0';
			upper(cp);
			p = lookup(sitetab, cp);
			cbuf[cpos] = c;
			if (p != 0) {
				if (p->implemented != 1 ||
				    (guest && p->guestokay == 0)) {
					state = CMD;
					nack(p->name);
					longjmp(errcatch,0);
					/* NOTREACHED */
				}
				state = p->state;
				yylval.s = p->name;
				return (p->token);
			}
			state = CMD;
			break;

		case OSTR:
			if (cbuf[cpos] == '\n') {
				state = CMD;
				return (CRLF);
			}
			/* FALLTHROUGH */

		case STR1:
		case ZSTR1:
		dostr1:
			if (cbuf[cpos] == ' ') {
				cpos++;
				state = state == OSTR ? STR2 : ++state;
				return (SP);
			}
			break;

		case ZSTR2:
			if (cbuf[cpos] == '\n') {
				state = CMD;
				return (CRLF);
			}
			/* FALLTHROUGH */

		case STR2:
			cp = &cbuf[cpos];
			n = strlen(cp);
			cpos += n - 1;
			/*
			 * Make sure the string is nonempty and \n terminated.
			 */
			if (n > 1 && cbuf[cpos] == '\n') {
				cbuf[cpos] = '\0';
				yylval.s = copy(cp);
				cbuf[cpos] = '\n';
				state = ARGS;
				return (STRING);
			}
			break;

		case NSTR:
			if (cbuf[cpos] == ' ') {
				cpos++;
				return (SP);
			}
			if (isdigit(cbuf[cpos])) {
				cp = &cbuf[cpos];
				while (isdigit(cbuf[++cpos]))
					;
				c = cbuf[cpos];
				cbuf[cpos] = '\0';
				yylval.i = atoi(cp);
				cbuf[cpos] = c;
				state = STR1;
				return (NUMBER);
			}
			state = STR1;
			goto dostr1;

		case ARGS:
			if (isdigit(cbuf[cpos])) {
				cp = &cbuf[cpos];
				while (isdigit(cbuf[++cpos]))
					;
				c = cbuf[cpos];
				cbuf[cpos] = '\0';
				yylval.i = atoi(cp);
				cbuf[cpos] = c;
				return (NUMBER);
			}
			switch (cbuf[cpos++]) {

			case '\n':
				state = CMD;
				return (CRLF);

			case ' ':
				return (SP);

			case ',':
				return (COMMA);

			case 'A':
			case 'a':
				return (A);

			case 'B':
			case 'b':
				return (B);

			case 'C':
			case 'c':
				return (C);

			case 'E':
			case 'e':
				return (E);

			case 'F':
			case 'f':
				return (F);

			case 'I':
			case 'i':
				return (I);

			case 'L':
			case 'l':
				return (L);

			case 'N':
			case 'n':
				return (N);

			case 'P':
			case 'p':
				return (P);

			case 'R':
			case 'r':
				return (R);

			case 'S':
			case 's':
				return (S);

			case 'T':
			case 't':
				return (T);

			}
			break;

		default:
			fatal("Unknown state in scanner.");
		}
		yyerror((char *) 0);
		state = CMD;
		longjmp(errcatch,0);
	}
}

void
upper(s)
	char *s;
{
	while (*s != '\0') {
		if (islower(*s))
			*s = toupper(*s);
		s++;
	}
}

static char *
copy(s)
	char *s;
{
	char *p;

	p = malloc((unsigned) strlen(s) + 1);
	if (p == NULL)
		fatal("Ran out of memory.");
	(void) strcpy(p, s);
	return (p);
}

static void
help(ctab, s)
	struct tab *ctab;
	char *s;
{
	struct tab *c;
	int width, NCMDS;
	char *type;

	if (ctab == sitetab)
		type = "SITE ";
	else
		type = "";
	width = 0, NCMDS = 0;
	for (c = ctab; c->name != NULL; c++) {
		int len = strlen(c->name);

		if (len > width)
			width = len;
		NCMDS++;
	}
	width = (width + 8) &~ 7;
	if (s == 0) {
		int i, j, w;
		int columns, lines;

		reply(-214, "The following %scommands are recognized %s.",
		    type, "(* =>'s unimplemented)");
		columns = 76 / width;
		if (columns == 0)
			columns = 1;
		lines = (NCMDS + columns - 1) / columns;
		for (i = 0; i < lines; i++) {
			printf("   ");
			for (j = 0; j < columns; j++) {
				c = ctab + j * lines + i;
				printf("%s%c", c->name,
					c->implemented == 1 ? ' ' : '*');
				if (c + lines >= &ctab[NCMDS])
					break;
				w = strlen(c->name) + 1;
				while (w < width) {
					putchar(' ');
					w++;
				}
			}
			printf("\r\n");
		}
		(void) fflush(stdout);
		reply(214, "Direct comments to ftp-bugs@%s.", OP(hostname));
		return;
	}
	upper(s);
	c = lookup(ctab, s);
	if (c == (struct tab *)0) {
		reply(502, "Unknown command %s.", s);
		return;
	}
	if (c->implemented == 1)
		reply(214, "Syntax: %s%s %s", type, c->name, c->help);
	else
		reply(214, "%s%-*s\t%s; unimplemented.", type, width,
		    c->name, c->help);
}

static void
sizecmd(filename)
	char *filename;
{
	switch (type) {
	case TYPE_L:
	case TYPE_I: {
		struct stat stbuf;
		if (stat(filename, &stbuf) < 0 || !S_ISREG(stbuf.st_mode))
			reply(550, "%s: not a plain file.", filename);
		else
			reply(213, "%qu", stbuf.st_size);
		break; }
	case TYPE_A: {
		FILE *fin;
		int c;
		off_t count;
		struct stat stbuf;
		fin = fopen(filename, "r");
		if (fin == NULL) {
			perror_reply(550, filename);
			return;
		}
		if (fstat(fileno(fin), &stbuf) < 0 || !S_ISREG(stbuf.st_mode)) {
			reply(550, "%s: not a plain file.", filename);
			(void) fclose(fin);
			return;
		}

		count = 0;
		while((c=getc(fin)) != EOF) {
			if (c == '\n')	/* will get expanded to \r\n */
				count++;
			count++;
		}
		(void) fclose(fin);

		reply(213, "%qd", count);
		break; }
	default:
		reply(504, "SIZE not implemented for Type %c.", "?AEIL"[type]);
	}
}

static int
port_ok(cmd)
	char *cmd;
{
	if ((!FL(PROXY_OK) &&
	    memcmp(&SIN(&data_dest)->sin_addr,
	    &SIN(&his_addr)->sin_addr,
	    sizeof(SIN(&data_dest)->sin_addr))) ||
	    (FL(RESTRICTEDPORTS) &&
	    ntohs(SIN(&data_dest)->sin_port) < IPPORT_RESERVED)) {
		usedefault = 1;
		reply(500, "Illegal PORT range rejected.");
		return (0);
	}
	return (1);
}
