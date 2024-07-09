/*	BSDI cmd1.c,v 2.7 1999/04/07 20:28:11 dab Exp	*/

/*-
 * Copyright (c) 1980, 1993
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
 */

#ifndef lint
static char sccsid[] = "@(#)cmd1.c	8.2 (Berkeley) 4/20/95";
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * User commands.
 */

/*
 * Print the current active headings.
 * Don't change dot if invoker didn't give an argument.
 */

static int screen;

int
headers(msgvec)
	int *msgvec;
{
	headers1(msgvec, 0);
}

int
del_headers(msgvec)
	int *msgvec;
{
	headers1(msgvec, MDELETED);
}

int
headers1(msgvec, del)
	int *msgvec;
	int del;
{
	register int n, mesg, flag;
	register struct message *mp;
	int size;

	size = screensize();
	n = msgvec[0];
	if (n != 0)
		screen = (n-1)/size;
	if (screen < 0)
		screen = 0;
	mp = &message[screen * size];
	if (mp >= &message[msgCount])
		mp = &message[msgCount - size];
	if (mp < &message[0])
		mp = &message[0];
	flag = 0;
	mesg = mp - &message[0];
	if (dot != &message[n-1])
		dot = mp;
	for (; mp < &message[msgCount]; mp++) {
		mesg++;
		if ((mp->m_flag^del) & MDELETED)
			continue;
		if (flag++ >= size)
			break;
		printhead(mesg);
	}
	if (flag == 0) {
		printf(del ? "No deleted messages.\n" : "No more mail.\n");
		return(1);
	}
	return(0);
}

/*
 * Scroll to the next/previous screen
 */
int
scroll(arg)
	char arg[];
{
	scroll1(arg, 0);
}

int
del_scroll(arg)
	char arg[];
{
	scroll1(arg, MDELETED);
}

int
scroll1(arg, del)
	char arg[];
	int del;
{
	register int s, size;
	int cur[1];

	cur[0] = 0;
	size = screensize();
	s = screen;
	switch (*arg) {
	case 0:
	case '+':
		s++;
		if (s * size > msgCount) {
			printf("On last screenful of messages\n");
			return(0);
		}
		screen = s;
		break;

	case '-':
		if (--s < 0) {
			printf("On first screenful of messages\n");
			return(0);
		}
		screen = s;
		break;

	default:
		printf("Unrecognized scrolling command \"%s\"\n", arg);
		return(1);
	}
	return(headers1(cur, del));
}

/*
 * Compute screen size.
 */
int
screensize()
{
	int s;
	char *cp;

	if ((cp = value("screen")) != NOSTR && (s = atoi(cp)) > 0)
		return s;
	return screenheight - 4;
}

/*
 * Print out the headlines for each message
 * in the passed message list.
 */
int
from(msgvec)
	int *msgvec;
{
	register int *ip;

	for (ip = msgvec; *ip != NULL; ip++)
		printhead(*ip);
	if (--ip >= msgvec)
		dot = &message[*ip - 1];
	return(0);
}

/*
 * Print out the header of a specific message.
 * This is a slight improvement to the standard one.
 */
void
printhead(mesg)
	int mesg;
{
	struct message *mp;
	char headline[LINESIZE], wcount[LINESIZE], *subjline, dispc, curind;
	char pbuf[BUFSIZ];
	struct headline hl;
	int subjlen;
	char *name;

	mp = &message[mesg-1];
	(void) readline(setinput(mp), headline, LINESIZE);
	if ((subjline = hfield("subject", mp)) == NOSTR)
		subjline = hfield("subj", mp);
	/*
	 * Bletch!
	 */
	curind = dot == mp ? '>' : ' ';
	dispc = ' ';
	if (mp->m_flag & MSAVED)
		dispc = '*';
	if (mp->m_flag & MPRESERVE)
		dispc = 'P';
	if ((mp->m_flag & (MREAD|MNEW)) == MNEW)
		dispc = 'N';
	if ((mp->m_flag & (MREAD|MNEW)) == 0)
		dispc = 'U';
	if (mp->m_flag & MBOX)
		dispc = 'M';
	parse(headline, &hl, pbuf);
	sprintf(wcount, "%3d/%-5ld", mp->m_lines, mp->m_size);
	subjlen = screenwidth - 50 - strlen(wcount);
	name = value("show-rcpt") != NOSTR ?
		skin(hfield("to", mp)) : nameof(mp, 0);
	if (subjline == NOSTR || subjlen < 0)		/* pretty pathetic */
		printf("%c%c%3d %-20.20s  %16.16s %s\n",
			curind, dispc, mesg, name, hl.l_date, wcount);
	else
		printf("%c%c%3d %-20.20s  %16.16s %s \"%.*s\"\n",
			curind, dispc, mesg, name, hl.l_date, wcount,
			subjlen, subjline);
}

/*
 * Print out the value of dot.
 */
int
pdot()
{
	printf("%d\n", dot - &message[0] + 1);
	return(0);
}

/*
 * Print out all the possible commands.
 */
int
pcmdlist()
{
	register struct cmd *cp;
	register int cc;
	extern struct cmd cmdtab[];

	printf("Commands are:\n");
	for (cc = 0, cp = cmdtab; cp->c_name != NULL; cp++) {
		cc += strlen(cp->c_name) + 2;
		if (cc > 72) {
			printf("\n");
			cc = strlen(cp->c_name) + 2;
		}
		if ((cp+1)->c_name != NOSTR)
			printf("%s, ", cp->c_name);
		else
			printf("%s\n", cp->c_name);
	}
	return(0);
}

/*
 * Paginate messages, honor ignored fields.
 */
int
more(msgvec)
	int *msgvec;
{
	return (type1(msgvec, 1, 1));
}

/*
 * Paginate messages, even printing ignored fields.
 */
int
More(msgvec)
	int *msgvec;
{

	return (type1(msgvec, 0, 1));
}

/*
 * Type out messages, honor ignored fields.
 */
int
type(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 1, 0));
}

/*
 * Type out messages, even printing ignored fields.
 */
int
Type(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 0, 0));
}

/*
 * Type out the messages requested.
 */
jmp_buf	pipestop;
int
type1(msgvec, doign, page)
	int *msgvec;
	int doign, page;
{
	register *ip;
	register struct message *mp;
	register char *cp;
	int nlines;
	FILE *obuf;
#ifndef NOMETAMAIL
	int PipeToMore = 0;
#endif

	obuf = stdout;
	if (setjmp(pipestop))
		goto close_pipe;
	if (value("interactive") != NOSTR &&
	    (page || (cp = value("crt")) != NOSTR)) {
		nlines = 0;
		if (!page) {
			for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++)
				nlines += message[*ip - 1].m_lines;
		}
		if (page || nlines > (*cp ? atoi(cp) : realscreenheight)) {
#ifndef NOMETAMAIL
			PipeToMore = 1;
#else
			cp = value("PAGER");
			if (cp == NULL || *cp == '\0')
				cp = _PATH_MORE;
			obuf = Popen(cp, "w");
			if (obuf == NULL) {
				perror(cp);
				obuf = stdout;
			} else
				signal(SIGPIPE, brokpipe);
#endif
		}
	}
	for (ip = msgvec; *ip && ip - msgvec < msgCount; ip++) {
		mp = &message[*ip - 1];
		touch(mp);
		dot = mp;
		if (value("quiet") == NOSTR)
			fprintf(obuf, "Message %d:\n", *ip);
#ifndef NOMETAMAIL
		if (value("metamail") != NOSTR && nontext(mp)) {
		    char Fname[100], Cmd[120];
		    FILE *fp;
		    int code;
		    struct termios ttystatein, ttystateout;

		    sprintf(Fname, "/tmp/metam.%05d", getpid());
		    fp = fopen(Fname, "w");
		    if (!fp) {
			perror(Fname);
		    } else {
			send(mp, fp, 0, NOSTR);
			fclose(fp);
			sprintf(Cmd, "metamail -z %s -m Mail %s", (PipeToMore) ? "-p" : "", Fname);
			if (obuf != stdout) {
			    pclose(obuf);
			    obuf = stdout;
			}
			tcgetattr(fileno(stdin), &ttystatein);
			tcgetattr(fileno(stdout), &ttystateout);
			code = system(Cmd);
			tcsetattr(fileno(stdin), TCSADRAIN, &ttystatein);
			tcsetattr(fileno(stdout), TCSADRAIN, &ttystateout);
#if 0
/*
 * The following line would cause the raw mail to print out if
 * metamail failed:
 */
			if (code) send(mp, obuf, doign ? ignore : 0, NOSTR);
#endif
		    }
		} else {
		    if (PipeToMore && stdout == obuf) {
			cp = value("PAGER");
			if (cp == NULL || *cp == '\0')
				cp = _PATH_MORE;
			obuf = Popen(cp, "w");
			if (obuf == NULL) {
				perror(cp);
				obuf = stdout;
			} else
				signal(SIGPIPE, brokpipe);
		    }
		    send(mp, obuf, doign ? ignore : 0, NOSTR);
		}
#else
		(void) send(mp, obuf, doign ? ignore : 0, NOSTR);
#endif
	}
close_pipe:
	if (obuf != stdout) {
		/*
		 * Ignore SIGPIPE so it can't cause a duplicate close.
		 */
		signal(SIGPIPE, SIG_IGN);
		Pclose(obuf);
		signal(SIGPIPE, SIG_DFL);
	}
	return(0);
}

/*
 * Respond to a broken pipe signal --
 * probably caused by quitting more.
 */
void
brokpipe(signo)
	int signo;
{
	longjmp(pipestop, 1);
}

/*
 * Print the top so many lines of each desired message.
 * The number of lines is taken from the variable "toplines"
 * and defaults to 5.
 */
int
top(msgvec)
	int *msgvec;
{
	register int *ip;
	register struct message *mp;
	int c, topl, lines, lineb;
	char *valtop, linebuf[LINESIZE];
	FILE *ibuf;

	topl = 5;
	valtop = value("toplines");
	if (valtop != NOSTR) {
		topl = atoi(valtop);
		if (topl < 0 || topl > 10000)
			topl = 5;
	}
	lineb = 1;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mp = &message[*ip - 1];
		touch(mp);
		dot = mp;
		if (value("quiet") == NOSTR)
			printf("Message %d:\n", *ip);
		ibuf = setinput(mp);
		c = mp->m_lines;
		if (!lineb)
			printf("\n");
		for (lines = 0; lines < c && lines <= topl; lines++) {
			if (readline(ibuf, linebuf, LINESIZE) < 0)
				break;
			puts(linebuf);
			lineb = blankline(linebuf);
		}
	}
	return(0);
}

/*
 * Touch all the given messages so that they will
 * get mboxed.
 */
int
stouch(msgvec)
	int msgvec[];
{
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag |= MTOUCH;
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * Make sure all passed messages get mboxed.
 */
int
mboxit(msgvec)
	int msgvec[];
{
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag |= MTOUCH|MBOX;
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * List the folders the user currently has.
 */
int
folders()
{
	char dirname[BUFSIZ];
	char *cmd;

	if (getfold(dirname) < 0) {
		printf("No value set for \"folder\"\n");
		return 1;
	}
	if ((cmd = value("LISTER")) == NOSTR)
		cmd = "ls";
	(void) run_command(cmd, 0, -1, -1, dirname, NOSTR, NOSTR);
	return (0);
}

/*
 * Update the mail file with any new messages that have
 * come in since we started reading mail.
 */
inc()
{
	int nmsg, mdot;

	nmsg = incfile();

	if (nmsg == 0)
		printf("No new mail.\n");
	else if (nmsg > 0) {
		mdot = newfileinfo(msgCount - nmsg);
		dot = &message[mdot - 1];
	} else
		printf("The \"inc\" command failed\n");

	return (0);
}

#ifndef NOMETAMAIL

nontext(mp) 
struct message *mp;
{
    long c;
    FILE *ibuf;
    char line[LINESIZE], *s, *t;

    ibuf = setinput(mp);
    c = mp->m_size;
    while (c > 0L) {
	fgets(line, LINESIZE, ibuf);
	c -= (long) strlen(line);
	if (line[0] == '\n') return(0);
	for (s=line; *s; ++s) if (isupper(*s)) *s = tolower(*s);
	if (!strncmp(line, "content-type:", 13) && notplain(line+13)) return(1);
    }
    return(0);
}
notplain(s)
char *s;
{
    char *t;
    if (!s) return(1);
    while (*s && isspace(*s)) ++s;
    for(t=s; *t; ++t) if (isupper(*t)) *t = tolower(*t);
    while (t > s && isspace(*--t)) {;}
    if (((t-s) == 3) && !strncmp(s, "text", 4)) return(0);
    if (strncmp(s, "text/plain", 10)) return(1);
    t = (char *) index(s, ';');
    while (t) {
        ++t;
        while (*t && isspace(*t)) ++t;
        if (!strncmp(t, "charset", 7)) {
            s = (char *) index(t, '=');
            if (s) {
                ++s;
                while (*s && isspace(*s)) ++s;
                if (*s == '"') ++s;
                if (!strncmp(s, "us-ascii", 8)) return(0);
            }
            return(1);
        }
        t = (char *) index(t, ';');
    }
    return(0); /* no charset, was text/plain */
}
#endif

/*
 * Get the relative date of a message for sorting
 */
int
get_date(mp)
	struct message *mp;
{
	char headline[LINESIZE];
	char pbuf[BUFSIZ];
	struct headline hl;

	(void) readline(setinput(mp), headline, LINESIZE);

	parse(headline, &hl, pbuf);
	return(num_date(hl.l_date));
}

/*
 * Date comparison function for sorting by "date"
 */
int
date_compare(m1, m2)
	struct message *m1, *m2;
{
	return(get_date(m1) - get_date(m2));
}

char *hdr_field;

/*
 * Comparison based on a header field
 */
int
hdr_compare(m1, m2)
	struct message *m1, *m2;
{
	char tbuf[LINESIZE];
	char *s1, *s2;
	char *s, *d;

	if ((s1 = hfield(hdr_field, m1)) != NULL) {
		strncpy(tbuf, s1, sizeof(tbuf));
		s1 = tbuf;
	}
	s2 = hfield(hdr_field, m2);

	return ((s1 == NULL) ? (s2 == NULL ? 0 : -1) :
	    (s2 == NULL) ? 1 : strncmp(s2, s1, LINESIZE));
}

/*
 * Compress a subject line prior to comparison for sorting:
 *	1) Remove all instances of "Re:"
 *	2) Remove leading whitespace
 *	3) Replace strings of whitespace with a single space. 
 */
void
compress_subj(str)
	char *str;
{
	char *s, *d;
	int need_space;

	d = s = str;

	need_space = 0;
	while (*s != '\0') {
		if (*s == ' ' || *s == '\t') {
			do
				s++;
			while (*s == ' ' || *s == '\t');
			if (d != str)
				need_space++;
		}
		if (raise(s[0]) == 'R' && raise(s[1]) == 'E' &&
		    raise(s[2]) == ':') {
			s+= 3;
			continue;
		}
		if (need_space) {
			*d++ = ' ';
			need_space = 0;
		}
		*d++ = *s++;
	}
	*d = '\0';
	return;
}

/*
 * Comparison based on a compressed subject line.
 */
int
subj_compare(m1, m2)
	struct message *m1, *m2;
{
	char tbuf[LINESIZE];
	char *s1, *s2;

	if ((s1 = hfield("subject", m1)) != NULL) {
		strncpy(tbuf, s1, sizeof(tbuf));
		s1 = tbuf;
	}
	s2 = hfield("subject", m2);

	if (s1 == NULL)
		return ((s2 == NULL) ? 0 : -1);
	if (s2 == NULL)
		return (1);

	compress_subj(s1);
	compress_subj(s2);

	return (strcmp(s1, s2));
}

int msgs_are_sorted = 0;

msg_sort(list)
	char *list[];
{
	struct message *mp;
	int (*func)();
	char *cp;
	char **what;

	if (msgCount <= 1)
		return;

	/* get to the end of the list, and sort backwards */
	for (what = list; *what; what++)
		;
	while (--what >= list) {
		if ((cp = strchr(*what, ':')) != NULL) {
			*cp = '\0';
			hdr_field = *what;
			func = hdr_compare;
		} else if (strcasecmp(*what, "date") == 0)
			func = date_compare;
		else if (strcasecmp(*what, "subject") == 0)
			func = subj_compare;
		else {
			printf("unknown sort criteria: %s\n", *what);
			continue;
		}
		mergesort(&message[0], msgCount, sizeof(message[0]), func);
	}
	dot = &message[0];
	msgs_are_sorted = 1;
}
