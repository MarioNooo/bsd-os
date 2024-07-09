/*
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
static char sccsid[] = "@(#)edit.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include "rcv.h"
#include <fcntl.h>
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Perform message editing functions.
 */

/*
 * Edit a message list.
 */
int
editor(msgvec)
	int *msgvec;
{

	return edit1(msgvec, 'e');
}

/*
 * Invoke the visual editor on a message list.
 */
int
visual(msgvec)
	int *msgvec;
{

	return edit1(msgvec, 'v');
}

/*
 * Edit a message by writing the message into a funnily-named file
 * (which should not exist) and forking an editor on it.
 * We get the editor from the stuff above.
 */
int
edit1(msgvec, type)
	int *msgvec;
	int type;
{
	register int c;
	int i;
	FILE *fp;
	register struct message *mp;
	off_t size;

	/*
	 * Deal with each message to be edited . . .
	 */
	for (i = 0; msgvec[i] && i < msgCount; i++) {
		sig_t sigint;

		if (i > 0) {
			char buf[100];
			char *p;

			printf("Edit message %d [ynq]? ", msgvec[i]);
			if (fgets(buf, sizeof buf, stdin) == 0)
				break;
			for (p = buf; *p == ' ' || *p == '\t'; p++)
				;
			if (*p == 'q')
				break;
			if (*p == 'n')
				continue;
		}
		dot = mp = &message[msgvec[i] - 1];
		touch(mp);
		sigint = signal(SIGINT, SIG_IGN);
		fp = run_editor(setinput(mp), mp->m_size, type, readonly);
		if (fp != NULL) {
			(void) fseek(otf, 0L, 2);
			size = ftell(otf);
			mp->m_block = blockof(size);
			mp->m_offset = offsetof(size);
			mp->m_size = fsize(fp);
			mp->m_lines = 0;
			mp->m_flag |= MODIFY;
			rewind(fp);
			while ((c = getc(fp)) != EOF) {
				if (c == '\n')
					mp->m_lines++;
				if (putc(c, otf) == EOF)
					break;
			}
			if (ferror(otf))
				perror("/tmp");
			(void) Fclose(fp);
		}
		(void) signal(SIGINT, sigint);
	}
	return 0;
}

/*
 * Run an editor on the file at "fpp" of "size" bytes,
 * and return a new file pointer.
 * Signals must be handled by the caller.
 * "Type" is 'e' for _PATH_EX, 'v' for _PATH_VI.
 */
FILE *
run_editor(fp, size, type, readonly)
	register FILE *fp;
	off_t size;
	int type, readonly;
{
	extern char tempEdit[];
	struct stat statb;
	FILE *nf;
	time_t modtime;
	int t, nnewlines;
	char *edit;

	/* Create and copy to the temporary file. */
	if ((t = open(tempEdit, O_CREAT | O_TRUNC | O_WRONLY,
	    readonly ? S_IRUSR : S_IRUSR | S_IWUSR)) < 0) {
		perror(tempEdit);
		return (NULL);
	}
	if ((nf = Fdopen(t, "w")) == NULL)
		goto ret1;
	if (size >= 0) {
		while (--size >= 0 && (t = getc(fp)) != EOF)
			if (putc(t, nf) == EOF)
				break;
	} else
		while ((t = getc(fp)) != EOF)
			if (putc(t, nf) == EOF)
				break;
	(void)fflush(nf);
	modtime = fstat(fileno(nf), &statb) < 0 ? 0 : statb.st_mtime;
	if (ferror(nf) || Fclose(nf) < 0)
		goto ret1;
	nf = NULL;

	/* Edit the file. */
	if ((edit = value(type == 'e' ? "EDITOR" : "VISUAL")) == NOSTR)
		edit = type == 'e' ? _PATH_EX : _PATH_VI;
	if (run_command(edit, 0, -1, -1, tempEdit, NOSTR, NOSTR) < 0)
		goto ret2;

	/* If in readonly mode or file unchanged, clean up and return. */
	if (readonly)
		goto ret2;
	if (stat(tempEdit, &statb) < 0)
		goto ret1;
	if (modtime == statb.st_mtime)
		goto ret2;

	/* Now, switch to the temporary file. */
	if ((nf = Fopen(tempEdit, "a+")) == NULL)
		goto ret1;

	/*
	 * If "size < 0", we are editing a message collected
	 * from standard input.  Thus, there is no need to force
	 * the trailing blank line, like we do when editing a
	 * message in the mail folder.
	 */
	if (size < 0)
		goto finish_up;

	/*
	 * Ensure that the tempEdit file ends with two newlines.  
	 *
	 * XXX
	 * Probably ought to have a `From' line, as well.
	 *
	 * XXX
	 * If the file is only a single byte long, the seek is going to
	 * fail, but I'm not sure we care.  In the case of any error, we
	 * pass back the file descriptor -- if we fail because the disk
	 * is too full, reads should continue to work and I see no reason
	 * to discard the user's work.
	 */
	if (fseek(nf, -2L, SEEK_END) < 0)
		return (nf);

	nnewlines = getc(nf) == '\n' ? 1 : 0;
	if (getc(nf) == '\n')
		++nnewlines;
	else
		nnewlines = 0;

	switch (nnewlines) {
	case 0:
		if (putc('\n', nf) == EOF)
			break;
		/* FALLTHROUGH */
	case 1:
		(void)putc('\n', nf);
		/* FALLTHROUGH */
	case 2:
		break;
	default:
		abort();
	}

finish_up:
	/* XXX: fflush is necessary, so future stat(2) succeeds. */
	(void)fflush(nf);

	(void)unlink(tempEdit);
	return (nf);

ret1:	perror(tempEdit);
ret2:	(void)unlink(tempEdit);
	if (nf != NULL)
		Fclose(nf);
	return (NULL);
}
