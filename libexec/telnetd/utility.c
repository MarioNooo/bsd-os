/*
 * Copyright (c) 1989, 1993
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
static char sccsid[] = "@(#)utility.c	8.4 (Berkeley) 5/30/95";
#endif /* not lint */

#define PRINTOPTIONS
#include <stdarg.h>
#include "telnetd.h"

/*
 * utility functions performing io related tasks
 */

/*
 * ttloop
 *
 *	A small subroutine to flush the network output buffer, get some data
 * from the network, and pass it through the telnet state machine.  We
 * also flush the pty input buffer (by dropping its data) if it becomes
 * too full.
 */

    void
ttloop()
{

    DIAG(TD_REPORT, net_printf("td: ttloop\r\n"));
    if (nfrontp - nbackp > 0) {
	netflush();
    }
    ncc = read(net, netibuf, sizeof netibuf);
    if (ncc < 0) {
	syslog(LOG_INFO, "ttloop:  read: %m\n");
	exit(1);
    } else if (ncc == 0) {
	syslog(LOG_INFO, "ttloop:  peer died: %m\n");
	exit(1);
    }
    DIAG(TD_REPORT, net_printf("td: ttloop read %d chars\r\n", ncc));
    netip = netibuf;
    telrcv();			/* state machine */
    if (ncc > 0) {
	pfrontp = pbackp = ptyobuf;
	telrcv();
    }
}  /* end of ttloop */

/*
 * Check a descriptor to see if out of band data exists on it.
 */
    int
stilloob(s)
    int	s;		/* socket number */
{
    static struct timeval timeout = { 0 };
    fd_set	excepts;
    int value;

    do {
	FD_ZERO(&excepts);
	FD_SET(s, &excepts);
	value = select(s+1, (fd_set *)0, (fd_set *)0, &excepts, &timeout);
    } while ((value == -1) && (errno == EINTR));

    if (value < 0) {
	fatalperror(pty, "select");
    }
    if (FD_ISSET(s, &excepts)) {
	return 1;
    } else {
	return 0;
    }
}

	void
ptyflush()
{
	int n;

	if ((n = pfrontp - pbackp) > 0) {
		DIAG(TD_REPORT | TD_PTYDATA,
		    net_printf("td: ptyflush %d chars\r\n", n));
		DIAG(TD_PTYDATA, printdata("pd", pbackp, n));
		n = write(pty, pbackp, n);
	}
	if (n < 0) {
		if (errno == EWOULDBLOCK || errno == EINTR)
			return;
		cleanup(0);
	}
	pbackp += n;
	if (pbackp == pfrontp)
		pbackp = pfrontp = ptyobuf;
}

/*
 * nextitem()
 *
 *	Return the address of the next "item" in the TELNET data
 * stream.  This will be the address of the next character if
 * the current address is a user data character, or it will
 * be the address of the character following the TELNET command
 * if the current address is a TELNET IAC ("I Am a Command")
 * character.
 */
    char *
nextitem(current)
    char	*current;
{
    if ((*current&0xff) != IAC) {
	return current+1;
    }
    switch (*(current+1)&0xff) {
    case DO:
    case DONT:
    case WILL:
    case WONT:
	return current+3;
    case SB:		/* loop forever looking for the SE */
	{
	    register char *look = current+2;

	    for (;;) {
		if ((*look++&0xff) == IAC) {
		    if ((*look++&0xff) == SE) {
			return look;
		    }
		}
	    }
	}
    default:
	return current+2;
    }
}  /* end of nextitem */


/*
 * netclear()
 *
 *	We are about to do a TELNET SYNCH operation.  Clear
 * the path to the network.
 *
 *	Things are a bit tricky since we may have sent the first
 * byte or so of a previous TELNET command into the network.
 * So, we have to scan the network buffer from the beginning
 * until we are up to where we want to be.
 *
 *	A side effect of what we do, just to keep things
 * simple, is to clear the urgent data pointer.  The principal
 * caller should be setting the urgent data pointer AFTER calling
 * us in any case.
 */
    void
netclear()
{
    register char *thisitem, *next;
    char *good;
#define	wewant(p)	((nfrontp > p) && ((*p&0xff) == IAC) && \
				((*(p+1)&0xff) != EC) && ((*(p+1)&0xff) != EL))

#ifdef	ENCRYPTION
    thisitem = nclearto > netobuf ? nclearto : netobuf;
#else	/* ENCRYPTION */
    thisitem = netobuf;
#endif	/* ENCRYPTION */

    while ((next = nextitem(thisitem)) <= nbackp) {
	thisitem = next;
    }

    /* Now, thisitem is first before/at boundary. */

#ifdef	ENCRYPTION
    good = nclearto > netobuf ? nclearto : netobuf;
#else	/* ENCRYPTION */
    good = netobuf;	/* where the good bytes go */
#endif	/* ENCRYPTION */

    while (nfrontp > thisitem) {
	if (wewant(thisitem)) {
	    int length;

	    next = thisitem;
	    do {
		next = nextitem(next);
	    } while (wewant(next) && (nfrontp > next));
	    length = next-thisitem;
	    memmove(good, thisitem, length);
	    good += length;
	    thisitem = next;
	} else {
	    thisitem = nextitem(thisitem);
	}
    }

    nbackp = netobuf;
    nfrontp = good;		/* next byte to be sent */
    neturg = 0;
}  /* end of netclear */

/*
 *  netflush
 *		Send as much data as possible to the network,
 *	handling requests for urgent data.
 */
    void
netflush()
{
    int n;
    extern int not42;

    while ((n = nfrontp - nbackp) > 0) {
	/* Write directly to the output buffer to avoid recursion with net_printf() */
	DIAG(TD_REPORT,
	    { if (&netobuf[BUFSIZ] - nfrontp > 26) {
		sprintf(nfrontp, "td: netflush %d chars\r\n", n);
		n += strlen(nfrontp);  /* get count first */
		nfrontp += strlen(nfrontp);  /* then move pointer */
	      }
	    });
#ifdef	ENCRYPTION
	if (encrypt_output) {
		char *s = nclearto ? nclearto : nbackp;
		if (nfrontp - s > 0) {
			(*encrypt_output)((unsigned char *)s, nfrontp-s);
			nclearto = nfrontp;
		}
	}
#endif	/* ENCRYPTION */
	/*
	 * if no urgent data, or if the other side appears to be an
	 * old 4.2 client (and thus unable to survive TCP urgent data),
	 * write the entire buffer in non-OOB mode.
	 */
	if ((neturg == 0) || (not42 == 0)) {
	    n = write(net, nbackp, n);	/* normal write */
	} else {
	    n = neturg - nbackp;
	    /*
	     * In 4.2 (and 4.3) systems, there is some question about
	     * what byte in a sendOOB operation is the "OOB" data.
	     * To make ourselves compatible, we only send ONE byte
	     * out of band, the one WE THINK should be OOB (though
	     * we really have more the TCP philosophy of urgent data
	     * rather than the Unix philosophy of OOB data).
	     */
	    if (n > 1) {
		n = send(net, nbackp, n-1, 0);	/* send URGENT all by itself */
	    } else {
		n = send(net, nbackp, n, MSG_OOB);	/* URGENT data */
	    }
	}
	if (n < 0) {
	    if (errno == EWOULDBLOCK || errno == EINTR)
		continue;
	    cleanup(0);
	    /* NOTREACHED */
	}
	nbackp += n;
#ifdef	ENCRYPTION
	if (nbackp > nclearto)
	    nclearto = 0;
#endif	/* ENCRYPTION */
	if (nbackp >= neturg) {
	    neturg = 0;
	}
	if (nbackp == nfrontp) {
	    nbackp = nfrontp = netobuf;
#ifdef	ENCRYPTION
	    nclearto = 0;
#endif	/* ENCRYPTION */
	}
    }
    return;
}  /* end of netflush */


/*
 * This function appends data to nfrontp and advances nfrontp.
 * Returns the number of characters written altogether (the
 * buffer may have been flushed in the process).
 */

int
net_printf(const char *format, ...)
{
	va_list args;
	int len;
	char buf[BUFSIZ];

	va_start(args, format);
	if ((len = vsnprintf(buf, sizeof(buf), format, args)) == -1)
		return -1;
	net_write(buf, len);
	va_end(args);
	return (len);
}  /* end of net_printf */

int
net_write(const char *buf, int len)
{
	int remaining, copied, olen = len;
	
	remaining = BUFSIZ - (nfrontp - netobuf);
	while (len > 0) {
		/* Free up enough space if the room is too low*/
		if ((len > BUFSIZ ? BUFSIZ : len) > remaining) {
			netflush();
			remaining = BUFSIZ - (nfrontp - netobuf);
		}

		/* Copy out as much as will fit */
		copied = remaining > len ? len : remaining;
		memmove(nfrontp, buf, copied);
		nfrontp += copied;
		len -= copied;
		remaining -= copied;
		buf += copied;
	}
	return (olen);
}  /* end of net_write */


/*
 * miscellaneous functions doing a variety of little jobs follow ...
 */


	void
fatal(f, msg)
	int f;
	char *msg;
{
	char buf[BUFSIZ];

	(void) snprintf(buf, sizeof(buf), "telnetd: %s.\r\n", msg);
#ifdef	ENCRYPTION
	if (encrypt_output) {
		/*
		 * Better turn off encryption first....
		 * Hope it flushes...
		 */
		encrypt_send_end();
		netflush();
	}
#endif	/* ENCRYPTION */
	(void) write(f, buf, (int)strlen(buf));
	sleep(1);	/*XXX*/
	exit(1);
}

	void
fatalperror(f, msg)
	int f;
	char *msg;
{
	char buf[BUFSIZ], *strerror();

	(void) snprintf(buf, sizeof(buf), "%s: %s", msg, strerror(errno));
	fatal(f, buf);
}

char editedhost[32];

	void
edithost(pat, host)
	register char *pat;
	register char *host;
{
	register char *res = editedhost;
	char *strncpy();

	if (!pat)
		pat = "";
	while (*pat) {
		switch (*pat) {

		case '#':
			if (*host)
				host++;
			break;

		case '@':
			if (*host)
				*res++ = *host++;
			break;

		default:
			*res++ = *pat;
			break;
		}
		if (res == &editedhost[sizeof editedhost - 1]) {
			*res = '\0';
			return;
		}
		pat++;
	}
	if (*host)
		(void) strncpy(res, host,
				sizeof editedhost - (res - editedhost) -1);
	else
		*res = '\0';
	editedhost[sizeof editedhost - 1] = '\0';
}

static char *putlocation;

	void
putstr(s)
	register char *s;
{

	while (*s)
		putchr(*s++);
}

	void
putchr(cc)
	int cc;
{
	*putlocation++ = cc;
}

/*
 * This is split on two lines so that SCCS will not see the M
 * between two % signs and expand it...
 */
static char fmtstr[] = { "%l:%M\
%P on %A, %d %B %Y" };

	void
putf(cp, where)
	register char *cp;
	char *where;
{
	char *slash;
	time_t t;
	char db[100];
#ifdef	STREAMSPTY
	extern char *strchr();
#else
	extern char *strrchr();
#endif

	putlocation = where;

	while (*cp) {
		if (*cp != '%') {
			putchr(*cp++);
			continue;
		}
		switch (*++cp) {

		case 't':
#ifdef	STREAMSPTY
			/* names are like /dev/pts/2 -- we want pts/2 */
			slash = strchr(line+1, '/');
#else
			slash = strrchr(line, '/');
#endif
			if (slash == (char *) 0)
				putstr(line);
			else
				putstr(&slash[1]);
			break;

		case 'h':
			putstr(editedhost);
			break;

		case 'd':
			(void)time(&t);
			(void)strftime(db, sizeof(db), fmtstr, localtime(&t));
			putstr(db);
			break;

		case '%':
			putchr('%');
			break;
		}
		cp++;
	}
}

#ifdef DIAGNOSTICS
/*
 * Print telnet options and commands in plain text, if possible.
 */
	void
printoption(fmt, option)
	register char *fmt;
	register int option;
{
	if (TELOPT_OK(option))
		net_printf("%s %s\r\n", fmt, TELOPT(option));
	else if (TELCMD_OK(option))
		net_printf("%s %s\r\n", fmt, TELCMD(option));
	else
		net_printf("%s %d\r\n", fmt, option);
	return;
}

    void
printsub(direction, pointer, length)
    char		direction;	/* '<' or '>' */
    unsigned char	*pointer;	/* where suboption data sits */
    int			length;		/* length of suboption data */
{
    register int i;
    char buf[512];

	if (!(diagnostic & TD_OPTIONS))
		return;

	if (direction) {
	    net_printf("td: %s suboption ",
					direction == '<' ? "recv" : "send");
	    if (length >= 3) {
		register int j;

		i = pointer[length-2];
		j = pointer[length-1];

		if (i != IAC || j != SE) {
		    net_printf("(terminated by ");
		    if (TELOPT_OK(i))
			net_printf("%s ", TELOPT(i));
		    else if (TELCMD_OK(i))
			net_printf("%s ", TELCMD(i));
		    else
			net_printf("%d ", i);
		    if (TELOPT_OK(j))
			net_printf("%s", TELOPT(j));
		    else if (TELCMD_OK(j))
			net_printf("%s", TELCMD(j));
		    else
			net_printf("%d", j);
		    net_printf(", not IAC SE!) ");
		}
	    }
	    length -= 2;
	}
	if (length < 1) {
	    net_printf("(Empty suboption??\?)");
	    return;
	}
	switch (pointer[0]) {
	case TELOPT_TTYPE:
	    net_printf("TERMINAL-TYPE ");
	    switch (pointer[1]) {
	    case TELQUAL_IS:
		net_printf("IS \"%.*s\"", length-2, (char *)pointer+2);
		break;
	    case TELQUAL_SEND:
		net_printf("SEND");
		break;
	    default:
		net_printf("- unknown qualifier %d (0x%x).",
				pointer[1], pointer[1]);
	    }
	    break;
	case TELOPT_TSPEED:
	    net_printf("TERMINAL-SPEED");
	    if (length < 2) {
		net_printf(" (empty suboption??\?)");
		break;
	    }
	    switch (pointer[1]) {
	    case TELQUAL_IS:
		net_printf(" IS %.*s", length-2, (char *)pointer+2);
		break;
	    default:
		if (pointer[1] == 1)
		    net_printf(" SEND");
		else
		    net_printf(" %d (unknown)", pointer[1]);
		for (i = 2; i < length; i++) {
		    net_printf(" ?%d?", pointer[i]);
		}
		break;
	    }
	    break;

	case TELOPT_LFLOW:
	    net_printf("TOGGLE-FLOW-CONTROL");
	    if (length < 2) {
		net_printf(" (empty suboption??\?)");
		break;
	    }
	    switch (pointer[1]) {
	    case LFLOW_OFF:
		net_printf(" OFF"); break;
	    case LFLOW_ON:
		net_printf(" ON"); break;
	    case LFLOW_RESTART_ANY:
		net_printf(" RESTART-ANY"); break;
	    case LFLOW_RESTART_XON:
		net_printf(" RESTART-XON"); break;
	    default:
		net_printf(" %d (unknown)", pointer[1]);
	    }
	    for (i = 2; i < length; i++) {
		net_printf(" ?%d?", pointer[i]);
	    }
	    break;

	case TELOPT_NAWS:
	    net_printf("NAWS");
	    if (length < 2) {
		net_printf(" (empty suboption??\?)");
		break;
	    }
	    if (length == 2) {
		net_printf(" ?%d?", pointer[1]);
		break;
	    }
	    net_printf(" %d %d (%d)",
		pointer[1], pointer[2],
		(int)((((unsigned int)pointer[1])<<8)|((unsigned int)pointer[2])));
	    if (length == 4) {
		net_printf(" ?%d?", pointer[3]);
		break;
	    }
	    net_printf(" %d %d (%d)",
		pointer[3], pointer[4],
		(int)((((unsigned int)pointer[3])<<8)|((unsigned int)pointer[4])));
	    for (i = 5; i < length; i++) {
		net_printf(" ?%d?", pointer[i]);
	    }
	    break;

	case TELOPT_LINEMODE:
	    net_printf("LINEMODE ");
	    if (length < 2) {
		net_printf(" (empty suboption??\?)");
		break;
	    }
	    switch (pointer[1]) {
	    case WILL:
		net_printf("WILL ");
		goto common;
	    case WONT:
		net_printf("WONT ");
		goto common;
	    case DO:
		net_printf("DO ");
		goto common;
	    case DONT:
		net_printf("DONT ");
	    common:
		if (length < 3) {
		    net_printf("(no option??\?)");
		    break;
		}
		switch (pointer[2]) {
		case LM_FORWARDMASK:
		    net_printf("Forward Mask");
		    for (i = 3; i < length; i++) {
			net_printf(" %x", pointer[i]);
		    }
		    break;
		default:
		    net_printf("%d (unknown)", pointer[2]);
		    for (i = 3; i < length; i++) {
			net_printf(" %d", pointer[i]);
		    }
		    break;
		}
		break;

	    case LM_SLC:
		net_printf("SLC");
		for (i = 2; i < length - 2; i += 3) {
		    if (SLC_NAME_OK(pointer[i+SLC_FUNC]))
			net_printf(" %s", SLC_NAME(pointer[i+SLC_FUNC]));
		    else
			net_printf(" %d", pointer[i+SLC_FUNC]);
		    switch (pointer[i+SLC_FLAGS]&SLC_LEVELBITS) {
		    case SLC_NOSUPPORT:
			net_printf(" NOSUPPORT"); break;
		    case SLC_CANTCHANGE:
			net_printf(" CANTCHANGE"); break;
		    case SLC_VARIABLE:
			net_printf(" VARIABLE"); break;
		    case SLC_DEFAULT:
			net_printf(" DEFAULT"); break;
		    }
		    net_printf("%s%s%s",
			pointer[i+SLC_FLAGS]&SLC_ACK ? "|ACK" : "",
			pointer[i+SLC_FLAGS]&SLC_FLUSHIN ? "|FLUSHIN" : "",
			pointer[i+SLC_FLAGS]&SLC_FLUSHOUT ? "|FLUSHOUT" : "");
		    if (pointer[i+SLC_FLAGS]& ~(SLC_ACK|SLC_FLUSHIN|
						SLC_FLUSHOUT| SLC_LEVELBITS)) {
			net_printf("(0x%x)", pointer[i+SLC_FLAGS]);
		    }
		    net_printf(" %d;", pointer[i+SLC_VALUE]);
		    if ((pointer[i+SLC_VALUE] == IAC) &&
			(pointer[i+SLC_VALUE+1] == IAC))
				i++;
		}
		for (; i < length; i++) {
		    net_printf(" ?%d?", pointer[i]);
		}
		break;

	    case LM_MODE:
		net_printf("MODE ");
		if (length < 3) {
		    net_printf("(no mode??\?)");
		    break;
		}
		{
		    char tbuf[32];
		    sprintf(tbuf, "%s%s%s%s%s",
			pointer[2]&MODE_EDIT ? "|EDIT" : "",
			pointer[2]&MODE_TRAPSIG ? "|TRAPSIG" : "",
			pointer[2]&MODE_SOFT_TAB ? "|SOFT_TAB" : "",
			pointer[2]&MODE_LIT_ECHO ? "|LIT_ECHO" : "",
			pointer[2]&MODE_ACK ? "|ACK" : "");
		    net_printf("%s", tbuf[1] ? &tbuf[1] : "0");
		}
		if (pointer[2]&~(MODE_EDIT|MODE_TRAPSIG|MODE_ACK)) {
		    net_printf(" (0x%x)", pointer[2]);
		}
		for (i = 3; i < length; i++) {
		    net_printf(" ?0x%x?", pointer[i]);
		}
		break;
	    default:
		net_printf("%d (unknown)", pointer[1]);
		for (i = 2; i < length; i++) {
		    net_printf(" %d", pointer[i]);
		}
	    }
	    break;

	case TELOPT_STATUS: {
	    register char *cp;
	    register int j, k;

	    net_printf("STATUS");

	    switch (pointer[1]) {
	    default:
		if (pointer[1] == TELQUAL_SEND)
		    net_printf(" SEND");
		else
		    net_printf(" %d (unknown)", pointer[1]);
		for (i = 2; i < length; i++) {
		    net_printf(" ?%d?", pointer[i]);
		}
		break;
	    case TELQUAL_IS:
		net_printf(" IS\r\n");

		for (i = 2; i < length; i++) {
		    switch(pointer[i]) {
		    case DO:	cp = "DO"; goto common2;
		    case DONT:	cp = "DONT"; goto common2;
		    case WILL:	cp = "WILL"; goto common2;
		    case WONT:	cp = "WONT"; goto common2;
		    common2:
			i++;
			if (TELOPT_OK(pointer[i]))
			    net_printf(" %s %s", cp, TELOPT(pointer[i]));
			else
			    net_printf(" %s %d", cp, pointer[i]);

			net_printf("\r\n");
			break;

		    case SB:
			net_printf(" SB ");
			i++;
			j = k = i;
			while (j < length) {
			    if (pointer[j] == SE) {
				if (j+1 == length)
				    break;
				if (pointer[j+1] == SE)
				    j++;
				else
				    break;
			    }
			    pointer[k++] = pointer[j++];
			}
			printsub(0, &pointer[i], k - i);
			if (i < length) {
			    net_printf(" SE");
			    i = j;
			} else
			    i = j - 1;

			net_printf("\r\n");

			break;

		    default:
			net_printf(" %d", pointer[i]);
			break;
		    }
		}
		break;
	    }
	    break;
	  }

	case TELOPT_XDISPLOC:
	    net_printf("X-DISPLAY-LOCATION ");
	    switch (pointer[1]) {
	    case TELQUAL_IS:
		net_printf("IS \"%.*s\"", length-2, (char *)pointer+2);
		break;
	    case TELQUAL_SEND:
		net_printf("SEND");
		break;
	    default:
		net_printf("- unknown qualifier %d (0x%x).",
				pointer[1], pointer[1]);
	    }
	    break;

	case TELOPT_NEW_ENVIRON:
	    net_printf("NEW-ENVIRON ");
	    goto env_common1;
	case TELOPT_OLD_ENVIRON:
	    net_printf("OLD-ENVIRON");
	env_common1:
	    switch (pointer[1]) {
	    case TELQUAL_IS:
		net_printf("IS ");
		goto env_common;
	    case TELQUAL_SEND:
		net_printf("SEND ");
		goto env_common;
	    case TELQUAL_INFO:
		net_printf("INFO ");
	    env_common:
		{
		    register int noquote = 2;
		    for (i = 2; i < length; i++ ) {
			switch (pointer[i]) {
			case NEW_ENV_VAR:
			    net_printf("\" VAR " + noquote);
			    noquote = 2;
			    break;

			case NEW_ENV_VALUE:
			    net_printf("\" VALUE " + noquote);
			    noquote = 2;
			    break;

			case ENV_ESC:
			    net_printf("\" ESC " + noquote);
			    noquote = 2;
			    break;

			case ENV_USERVAR:
			    net_printf("\" USERVAR " + noquote);
			    noquote = 2;
			    break;

			default:
			def_case:
			    if (isprint(pointer[i]) && pointer[i] != '"') {
				if (noquote) {
				    net_write("\"", 1);
				    noquote = 0;
				}
				net_printf("%c", pointer[i]);
			    } else {
				net_printf("\" %03o " + noquote,
							pointer[i]);
				noquote = 2;
			    }
			    break;
			}
		    }
		    if (!noquote)
			net_write("\"", 1);
		    break;
		}
	    }
	    break;

#if	defined(AUTHENTICATION)
	case TELOPT_AUTHENTICATION:
	    net_printf("AUTHENTICATION");

	    if (length < 2) {
		net_printf(" (empty suboption??\?)");
		break;
	    }
	    switch (pointer[1]) {
	    case TELQUAL_REPLY:
	    case TELQUAL_IS:
		net_printf(" %s ", (pointer[1] == TELQUAL_IS) ?
							"IS" : "REPLY");
		if (AUTHTYPE_NAME_OK(pointer[2]))
		    net_printf("%s ", AUTHTYPE_NAME(pointer[2]));
		else
		    net_printf("%d ", pointer[2]);
		if (length < 3) {
		    net_printf("(partial suboption??\?)");
		    break;
		}
		net_printf("%s|%s",
			((pointer[3] & AUTH_WHO_MASK) == AUTH_WHO_CLIENT) ?
			"CLIENT" : "SERVER",
			((pointer[3] & AUTH_HOW_MASK) == AUTH_HOW_MUTUAL) ?
			"MUTUAL" : "ONE-WAY");

		auth_printsub(&pointer[1], length - 1, buf, sizeof(buf));
		net_printf("%s", buf);
		break;

	    case TELQUAL_SEND:
		i = 2;
		net_printf(" SEND ");
		while (i < length) {
		    if (AUTHTYPE_NAME_OK(pointer[i]))
			net_printf("%s ", AUTHTYPE_NAME(pointer[i]));
		    else
			net_printf("%d ", pointer[i]);
		    if (++i >= length) {
			net_printf("(partial suboption??\?)");
			break;
		    }
		    net_printf("%s|%s ",
			((pointer[i] & AUTH_WHO_MASK) == AUTH_WHO_CLIENT) ?
							"CLIENT" : "SERVER",
			((pointer[i] & AUTH_HOW_MASK) == AUTH_HOW_MUTUAL) ?
							"MUTUAL" : "ONE-WAY");
		    ++i;
		}
		break;

	    case TELQUAL_NAME:
		net_printf(" NAME \"%.*s\"", length - 2, pointer + 2); 
		break;

	    default:
		    for (i = 2; i < length; i++) {
			net_printf(" ?%d?", pointer[i]);
		    }
		    break;
	    }
	    break;
#endif

#ifdef	ENCRYPTION
	case TELOPT_ENCRYPT:
	    net_printf("ENCRYPT");
	    if (length < 2) {
		net_printf(" (empty suboption??\?)");
		break;
	    }
	    switch (pointer[1]) {
	    case ENCRYPT_START:
		net_printf(" START");
		break;

	    case ENCRYPT_END:
		net_printf(" END");
		break;

	    case ENCRYPT_REQSTART:
		net_printf(" REQUEST-START");
		break;

	    case ENCRYPT_REQEND:
		net_printf(" REQUEST-END");
		break;

	    case ENCRYPT_IS:
	    case ENCRYPT_REPLY:
		net_printf(" %s ", (pointer[1] == ENCRYPT_IS) ?
							"IS" : "REPLY");
		if (length < 3) {
		    net_printf(" (partial suboption??\?)");
		    break;
		}
		if (ENCTYPE_NAME_OK(pointer[2]))
		    net_printf("%s ", ENCTYPE_NAME(pointer[2]));
		else
		    net_printf(" %d (unknown)", pointer[2]);

		encrypt_printsub(&pointer[1], length - 1, buf, sizeof(buf));
		net_printf("%s", buf);
		break;

	    case ENCRYPT_SUPPORT:
		i = 2;
		net_printf(" SUPPORT ");
		while (i < length) {
		    if (ENCTYPE_NAME_OK(pointer[i]))
			net_printf("%s ", ENCTYPE_NAME(pointer[i]));
		    else
			net_printf("%d ", pointer[i]);
		    i++;
		}
		break;

	    case ENCRYPT_ENC_KEYID:
		net_printf(" ENC_KEYID", pointer[1]);
		goto encommon;

	    case ENCRYPT_DEC_KEYID:
		net_printf(" DEC_KEYID", pointer[1]);
		goto encommon;

	    default:
		net_printf(" %d (unknown)", pointer[1]);
	    encommon:
		for (i = 2; i < length; i++) {
		    net_printf(" %d", pointer[i]);
		}
		break;
	    }
	    break;
#endif	/* ENCRYPTION */

	default:
	    if (TELOPT_OK(pointer[0]))
		net_printf("%s (unknown)", TELOPT(pointer[0]));
	    else
		net_printf("%d (unknown)", pointer[i]);
	    for (i = 1; i < length; i++) {
		net_printf(" %d", pointer[i]);
	    }
	    break;
	}
	net_printf("\r\n");
}

/*
 * Dump a data buffer in hex and ascii to the output data stream.
 */
	void
printdata(tag, ptr, cnt)
	register char *tag;
	register char *ptr;
	register int cnt;
{
	register int i;
	char xbuf[30];

	while (cnt) {
		/* flush net output buffer if no room for new data) */
		if ((&netobuf[BUFSIZ] - nfrontp) < 80) {
			netflush();
		}

		/* add a line of output */
		net_printf("%s: ", tag);
		for (i = 0; i < 20 && cnt; i++) {
			net_printf("%02x", *ptr);
			if (isprint(*ptr)) {
				xbuf[i] = *ptr;
			} else {
				xbuf[i] = '.';
			}
			if (i % 2) {
				net_printf(" ");
			}
			cnt--;
			ptr++;
		}
		xbuf[i] = '\0';
		net_printf(" %s\r\n", xbuf );
	}
}
#endif /* DIAGNOSTICS */
