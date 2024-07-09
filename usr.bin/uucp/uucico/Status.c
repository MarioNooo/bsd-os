/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routines to manipulate call status files.
**
**	RCSID Status.c,v 2.2 1996/06/11 19:27:26 vixie Exp
**
**	Status.c,v
**	Revision 2.2  1996/06/11 19:27:26  vixie
**	2.2
**	
**	Revision 2.1  1995/02/03 13:21:23  polk
**	Update all revs to 2.1
**
 * Revision 1.3  1995/01/08  07:48:51  donn
 * Some changes from Paul Vixie.
 *
 * Revision 1.2  1994/01/31  01:26:42  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:54  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILES
#define	FILE_CONTROL
#define	NO_VA_FUNC_DECLS
#define	SETJMP
#define	SYS_TIME

#include	"global.h"
#include	"cico.h"
#include <assert.h>

/*
**	Files each have 2 lines:
**		node "in"  line oktime lasttime retrytime state count text...
**		node "out" line oktime lasttime retrytime state count text...
*/

static char	WriteFmt[]	= "%s %s %s %lu %lu %lu %d %d %s";
static char	ReadFmt[]	= "%*s %*s %*s %*lu %lu %lu %d %d %*s";
static char	NL[]		= "\n";

/*
**	Local variables.
*/

static char *	StatusFile;
static int	StatusFd	= SYSERROR;
static bool	StatusMode;

/*
**	Local routines.
*/

static void	close_status(void);
static bool	open_status(char *);


static int
ParseStatus(const char *outdata,
	    Time_t *lasttime,
	    Time_t *retrytime,
	    int *mode,
	    int *count)
{
	assert(outdata != NULL);
	assert(lasttime != NULL);
	assert(retrytime != NULL);
	assert(mode != NULL);
	assert(count != NULL);
	if (!*outdata) {
		*lasttime = Time;
		*retrytime = SysRetryTime;
		*mode = SS_OK;
		*count = 0;
		return (0);
	}
	if (4 == sscanf(outdata, ReadFmt, lasttime, retrytime, mode, count))
		return (0);
	return (-1);
}

/*
**	Find out if call can be made.
*/

StatusType
CallOK(
	char *	node
)
{
	int	count;
	char *	indata;
	Time_t	lasttime;
	int	mode;
	int	oldmode;
	char *	outdata;
	Time_t	retrytime;

	Trace((1, "CallOK(%s)", node));

	mode = (int)SS_OK;

	if ( !open_status(node) )
		return SS_INPROGRESS;	/* LOCKED */

	while ( (indata = ReadFd(StatusFd)) != NULLSTR )
	{
		if
		(
			(outdata = strchr(indata, '\n')) == NULLSTR
			||
			strchr(++outdata, '\n') != NULLSTR
		)
		{
			goto bad_format;
		}

		Trace((3, "scan line: %s", outdata));

		if (ParseStatus(outdata,
				&lasttime, &retrytime, &oldmode, &count)
		    == -1) {
 bad_format:
			DebugT(1, (1, "CallOK bad status format:\n%s",
				   ExpandString(indata, RdFileSize)));
			(void)unlink(StatusFile);
			free(indata);
			break;
		}

		switch ( (StatusType)oldmode )
		{
		default:
			break;

		case SS_WRONGTIME:
		case SS_FAIL:
			(void)SetTimes();
			if ( (Time - lasttime) >= retrytime )
				break;

			Debug((1, "NO CALL: RETRY TIME (%lu) NOT REACHED",
			       retrytime));

			if ( Debugflag )
				Debug((1, "debugging: continuing anyway"));
			else
				mode = (int)SS_WRONGTIME;
			break;
		}

		free(indata);
		break;
	}

	if ( mode != (int)SS_OK )
		close_status();

	Debug((2, "CallOK => %d", mode));
	return (StatusType)mode;
}

/*
**	Close status file.
*/

static void
close_status()
{
	Trace((1,
		"close_status() fd=%d [%s]",
		StatusFd,
		(StatusFile==NULLSTR) ? NullStr : StatusFile
	));

	FreeStr(&StatusFile);

	if ( StatusFd == SYSERROR )
		return;

	(void)close(StatusFd);
	StatusFd = SYSERROR;
}

/*
**	Open status file, and lock.
*/

static bool
open_status(
	char *	name
)
{
	Trace((3, "open_status(%s)", (name==NULLSTR) ? NullStr : name));

	if
	(
		name == NULLSTR
		||
		name[0] == '\0'
		||
		strcmp(name, NODENAME) == STREQUAL
	)
		return false;

	if ( StatusFd != SYSERROR )
	{
		(void)lseek(StatusFd, (off_t)0, 0);
		return true;
	}

	StatusFile = concat(STATUSDIR, name, NULLSTR);

	for (;;) {
		mode_t old_umask = umask(02);

		StatusFd = open(StatusFile, O_CREAT|O_RDWR, 0664);
		umask(old_umask);
		if (StatusFd != SYSERROR)
			break;
		if (!CheckDirs(StatusFile))
			SysError(CouldNot, CreateStr, StatusFile);
	}

	Trace((2, "open_status(%s) => %s", name, StatusFile));

	while ( flock(StatusFd, LOCK_EX|LOCK_NB) == SYSERROR )
	{
		if ( errno == EWOULDBLOCK )
		{
			close_status();
			return false;
		}

		SysError(CouldNot, LockStr, StatusFile);
	}

	fioclex(StatusFd);

	return true;
}

/*
**	Write status entry.
**
**	Status(StatusType, char *node, char *fmt, ...)
*/

bool
Status(va_alist)
	va_dcl
{
	register va_list vp;

	char *		buf;
	int		count;
	char *		data;
	char *		indata;
	int		mode;
	char *		node;
	char *		outdata;
	Time_t		retrytime;
	char *		str;
	char *		ttyn;
	StatusType	type;

	static Time_t	lasttime;

	va_start(vp);

	if ( (type = va_arg(vp, StatusType)) == SS_CLOSE )
	{
		close_status();
		va_end(vp);
		return true;
	}

	node = va_arg(vp, char *);
	str = newvprintf(vp);
	va_end(vp);

	Trace((1, "Status(%s, %s, %d)", node, str, type));

	if ( !open_status(node) )
	{
		free(str);
		return false;	/* LOCKED */
	}

	outdata = indata = EmptyStr;
	count = 0;
	mode = SS_OK;

	while ( (data = ReadFd(StatusFd)) != NULLSTR )
	{
		if ( (outdata = strchr(data, '\n')) == NULLSTR )
		{
			outdata = EmptyStr;
			break;	/* Bad format */
		}

		*outdata++ = '\0';
		indata = data;

		if ( InitialRole == MASTER )
			buf = outdata;
		else
			buf = indata;

		Trace((3, "scan line: %s", buf));

		if (ParseStatus(buf,
				&lasttime, &retrytime, &mode, &count) == -1) {
			outdata = EmptyStr;
			break;	/* Bad format */
		}

		if ( type == SS_WRONGTIME && mode != (int)SS_INPROGRESS )
		{
			free(data);
			free(str);
			return true;	/* Preserve old mode */
		}

		if ( count < 0 )
			count = 0;
		break;
	}

	if ( type != SS_CALLING )
	{
		if ( type == SS_OK )
			retrytime = SysRetryTime;

		if (type == SS_HALFWAY)
			count = 0;

		/** Semi-exponential backoff **/
		if (type == SS_FAIL || type == SS_HALFWAY) {
			if (!count++)
				retrytime = SysRetryTime;
			if ((retrytime += (retrytime / 2) + 1) > DAY)
				retrytime = DAY;
		}

		if ( (ttyn = TtyName) == NULLSTR || ttyn[0] == '\0' )
			ttyn = DFLTTTYNAME;

		(void)SetTimes();

		if ( type == SS_OK )
		{
			lasttime = Time;
			count = 0;
		}

		buf = newprintf
			(
				WriteFmt,
				RemoteNode,
				(InitialRole == MASTER) ? "out" : "in",
				ttyn,
				(type == SS_OK) ?(Ulong)Time :(Ulong)lasttime,
				(Ulong)Time,
				(Ulong)retrytime,
				(int)type,
				(int)count,
				str
			);
	
		if ( InitialRole == MASTER )
			outdata = buf;
		else
			indata = buf;

		Debug((1, "status => %s", buf));
	}
	else
		buf = NULLSTR;

	outdata = concat(indata, NL, outdata, NL, NULLSTR);

	(void)lseek(StatusFd, (off_t)0, 0);

	/** NB: if no `ftruncate(2)', then we must create/rename a temporary file. **/

	while ( ftruncate(StatusFd, (off_t)0) == SYSERROR )
		SysError(CouldNot, "truncate", StatusFile);

	while ( write(StatusFd, outdata, strlen(outdata)) == SYSERROR )
	{
		SysError(CouldNot, WriteStr, StatusFile);
		(void)lseek(StatusFd, (off_t)0, 0);
	}

	free(outdata);
	FreeStr(&buf);
	FreeStr(&data);
	free(str);

	return true;
}
