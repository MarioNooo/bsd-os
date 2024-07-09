/*	BSDI extern.h,v 2.2 1996/03/29 03:05:55 prb Exp	*/

/*
 * Copyright (c) 1993
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
 *	@(#)extern.h	8.1 (Berkeley) 6/4/93
 */


char	*autobaud __P((void));
void	 edithost __P((char *));
void	 gendefaults __P((void));
void	 get_ttydefaults __P((int, struct termios *));
int	 getent __P((char *, char *));
int	 getflag __P((char *));
long	 getnum __P((char *));
char	*getstr __P((char *, char **));
void	 gettable __P((char *, char *));
void	 makeenv __P((char *[]));
char	*portselector __P((void));
void	 setchars __P((void));
void	 setdefaults __P((void));
void	 settmode __P((int, struct termios *, struct termios *));
int	 speed __P((int));

int	 test_clocal __P((int));
void	 set_clocal __P((int, struct termios *));
void	 flush_echos __P((int));
int	 seize_lock __P((char *, int));
void	 release_lock __P((char *));

int	 uu_unlock __P((char *));
int	 uu_lock __P((char *));

int	 login_tty __P((int));			/* From libutil. */