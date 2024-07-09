/* wv.c - windview event logging management */

/* includes */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include <sys/evtlog.h>

#include "wvUploadLib.h"
#include "wvUploadPathP.h"
#include "wvFileUploadPathLibP.h"
#include "wvSockUploadPathLibP.h"

/* defines */

#define EVT_LOG_DEV	"/dev/evtlog"

#define WV_DFLT_FILE	"/tmp/wvLog.wvr"
#define WV_DFLT_FLAG	(O_CREAT | O_TRUNC)
#define WV_DFLT_PORT	6164

#define WV_INST_LVL_OPTION_PARSE(usage)					\
    case 'l':	/* instrumentation level */				\
	setWvInst = 1;							\
	wvInst.class = strtoul (optarg, NULL, 0);			\
									\
	if (wvInst.class & (WV_CLASS_1 | WV_CLASS_2 | WV_CLASS_3) == 0)	\
	    {								\
	    printf ("Bad instrumentation level\n");			\
	    return (1);							\
	    }								\
	break;								\
    case 'o':	/* object instrumented */				\
	setWvInst = 1;							\
	wvInst.objInst = strtoul (optarg, NULL, 0);			\
	break;								\
    case 'p':								\
	setWvInst = 1;							\
	if (wvInst.id & EVT_LOG_INST_BYGROUP)				\
	    {								\
	    printf ("p and g options are mutually exclusive\n");	\
	    return (1);							\
	    }								\
	wvInst.id = strtoul (optarg, NULL, 0);				\
	wvInst.instOptions |= EVT_LOG_INST_BYPID;			\
	break;								\
    case 'g':	/* instrument processes form this group */		\
	setWvInst = 1;							\
	if (wvInst.id & EVT_LOG_INST_BYPID)				\
	    {								\
	    printf ("p and g options are mutually exclusive\n");	\
	    return (1);							\
	    }								\
	wvInst.id = strtoul (optarg, NULL, 0);				\
	wvInst.instOptions |= EVT_LOG_INST_BYGROUP;			\
	break;								\
    case 'd':	/* descend option */					\
	setWvInst = 1;							\
	wvInst.instOptions |= EVT_LOG_INST_DESCEND;			\
	break;								\
    case 'i': 	/* inherit option */					\
	setWvInst = 1;							\
	wvInst.instOptions |= EVT_LOG_INST_INHERIT;			\
	break;

#define WV_UPLOAD_OPTION_PARSE(usageRtn)				\
    case 'P':	/* upload path */					\
	if (strcmp (optarg, "FILE") == 0)				\
	    {								\
	    _func_wvOnUploadPathCreate = (FUNCPTR)fileUploadPathCreate;	\
	    _func_wvOnUploadPathClose = (FUNCPTR)fileUploadPathClose;	\
	    if (wvUploadPathArg1 == 0)					\
		wvUploadPathArg1 = (int)WV_DFLT_FILE;			\
	    if (wvUploadPathArg2 == 0)					\
		wvUploadPathArg2 = WV_DFLT_FLAG;			\
	    }								\
	else if (strcmp (optarg, "SOCKET") == 0)			\
	    {								\
	    _func_wvOnUploadPathCreate = (FUNCPTR)sockUploadPathCreate;	\
	    _func_wvOnUploadPathClose = (FUNCPTR)sockUploadPathClose;	\
	    if (wvUploadPathArg2 == 0)					\
		wvUploadPathArg2 = WV_DFLT_PORT;			\
	    }								\
	else								\
	    {								\
	    printf ("Unknown upload path\n");				\
	    usageRtn ();						\
	    return (1);							\
	    }								\
	break;								\
    case 'F':	/* FILE upload path filename */				\
	wvUploadPathArg1 = (int)optarg; break;				\
    case 'f':	/* FILE upload path flag */				\
	wvUploadPathArg2 = strtoul (optarg, NULL, 0); break;		\
    case 'S':	/* SOCKET upload path host IP address */		\
	wvUploadPathArg1 = (int)optarg; break;				\
    case 's':	/* SOCKET upload path port number */			\
	wvUploadPathArg2 = strtoul (optarg, NULL, 0); break;		\

#define WV_EVTLOG_OPEN(logFd, flag)			\
    if ((logFd = open (EVT_LOG_DEV, flag)) < 0)		\
    	{						\
	perror ("Can't open event logging device ("EVT_LOG_DEV"),\nmake sure evtlog pseudo-device is configured and SMP option is not.\n");\
	return 1;					\
	}

#define WV_EVTLOG_CLOSE_AND_RETURN(logFd, retVal)	\
    close (logFd);					\
    return (retVal);

#define WV_EVTLOG_CLASSSET(logFd, wvInst)			\
    if (setWvInst && ioctl (logFd, DHIOCEVTCLASSSET, &wvInst) != 0)	\
    	{								\
	perror ("Can't set logging level\n");				\
	WV_EVTLOG_CLOSE_AND_RETURN(logFd, 1);				\
	}

#define WV_DEFAULT_UPLOAD_PATH_SET					\
    if (_func_wvOnUploadPathCreate == NULL)				\
    	{								\
	fileUploadPathLibInit ();					\
	_func_wvOnUploadPathCreate = (FUNCPTR) fileUploadPathCreate;	\
	_func_wvOnUploadPathClose = (FUNCPTR) fileUploadPathClose;	\
	wvUploadPathArg1 = (int)WV_DFLT_FILE;				\
	wvUploadPathArg2 = WV_DFLT_FLAG;				\
	}

#define WV_UPLOAD_PATH_CREATE(logFd)					\
    if (_func_wvOnUploadPathCreate != NULL)				\
    	{								\
	wvUploadPathId = (UPLOAD_ID)_func_wvOnUploadPathCreate		\
					    ((char *)wvUploadPathArg1,	\
					     (short)wvUploadPathArg2);	\
	if (wvUploadPathId == NULL)					\
	    {								\
	    perror ("Can't create upload path\n");			\
	    close (logFd);						\
	    return (1);							\
	    }								\
	}
    
#define WV_UPLOAD_PATH_CLOSE				\
    if (_func_wvOnUploadPathClose != NULL)		\
    	_func_wvOnUploadPathClose (wvUploadPathId);

#define WV_CONTINUOUS_UPLOAD(logFd, fdSet, rBuffInfo)		\
    FD_ZERO(&fdSet);						\
    FD_SET(logFd, &fdSet);					\
    								\
    if ((select (FD_SETSIZE, &fdSet, 0, 0, 0) >= 0) &&		\
    	(FD_ISSET (logFd, &fdSet)))				\
	{							\
	if (wvUpload (logFd, wvUploadPathId, 0, 0, &rBuffInfo))	\
	    {							\
	    /* printf ("upload error exiting loop\n"); */	\
	    break;						\
	    }							\
	}							\
    else							\
	{							\
	/* printf ("Select error exiting loop\n"); */		\
    	break;							\
	}

/* locals */

static int			setWvInst = 0;
static rbuff_create_params_t	rBuffCreateParams;
static evtlog_inst_t		wvInst;

static UPLOAD_ID		wvUploadPathId = NULL;
static FUNCPTR			_func_wvOnUploadPathCreate;
static FUNCPTR			_func_wvOnUploadPathClose;
static int			wvArgHtons;
static int			wvUploadPathArg1;
static int			wvUploadPathArg2;

static int			destroyBuff = 0;
static int			resetBuff = 0;
static int			verbose = 0;
static int			uploadTest = 0;

static int			upCntMax;
static int			tgtCheck = 0;

/* forward declarations */

int	wvCmdLineParse (int argc, char * argv[]);
int	wvCfgLineParse (int argc, char * argv[]);
int	wvOnLineParse (int argc, char * argv[]);
int	wvOffLineParse (int argc, char * argv[]);
int	wvUploadLineParse (int argc, char * argv[]);

int	wvCfg (void);
int	wvOff (void);
int	wvOn (void);
int	wvUploadCmd (void);

void	wvUsage (void);
void	wvCfgUsage (void);
void	wvOnUsage (void);
void	wvOffUsage (void);
void	wvUploadUsage (void);

/*****************************************************************************
*
* main - entry point 
*
*/

int main (argc, argv)
    int		argc;
    char *	argv[];
    {
    if (wvCmdLineParse (argc, argv))
	return (1);

    return 0;
    }

/******************************************************************************
*
* wvCmdLineParse - 
*
*/

int wvCmdLineParse
    (
    int		argc,
    char *	argv[]
    )
    {
    /* determine command first (default is 'on') */

    if (argc > 1)
    	{
	if (strcmp (argv[1], "cfg") == 0)
	    {
	    if (wvCfgLineParse (--argc, ++argv) != 0)
	    	return (1);
	    return (wvCfg ());
	    }
	else if (strcmp (argv[1], "on") == 0)
	    {
	    if (wvOnLineParse (--argc, ++argv) != 0)
	    	return (1);
	    return (wvOn ());
	    }
	else if (strcmp (argv[1], "off") == 0)
	    {
	    if (wvOffLineParse (--argc, ++argv) != 0)
	    	return (1);
	    return (wvOff ());
	    }
	else if (strcmp (argv[1], "upload") == 0)
	    {
	    if (wvUploadLineParse (--argc, ++argv) != 0)
	    	return (1);
	    return (wvUploadCmd ());
	    }
	else
	    {
	    wvUsage ();
	    return 1;
	    }
	}
    else
    	{
	wvUsage ();
	return 1;
	}
    }

/*****************************************************************************
*
* wvCfgLineParse - parse config command options
*
* RETURNS: 0 or 1 if unsupported option is found
*
*/

int wvCfgLineParse 
    (
    int 	argc,
    char *	argv[]
    )
    {
    extern char *	optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    /* default values */

    rBuffCreateParams.maximum = 10;
    rBuffCreateParams.minimum = 2;
    rBuffCreateParams.buffSize = 0x4000;

    while ((ch = getopt (argc, argv, "dDhig:l:M:m:o:p:s:w")) != -1)
    	{
	switch (ch)
	    {
	    case 'D':	/* deferred mode */
	    	rBuffCreateParams.options |= RBUFF_UP_DEFERRED;
		break;
	    case 'M':
		rBuffCreateParams.maximum = strtoul (optarg, NULL, 0);
		break;
	    case 'm':
		rBuffCreateParams.minimum = strtoul (optarg, NULL, 0);
		break;
	    case 's':
	    	rBuffCreateParams.buffSize = strtoul (optarg, NULL, 0);
		break;
	    case 'w':
		rBuffCreateParams.options |= RBUFF_WRAPAROUND;
		break;
	    WV_INST_LVL_OPTION_PARSE(wvCfgUsage)
	    case 'h':
	    default :
	    	wvCfgUsage();
		return (1);
	    }
	}

    return (0);
    }

/*****************************************************************************
*
* wvOffLineParse - parse 'off' command options
*
* RETURNS: 0 or 1 if unsupported option is found
*
*/

int wvOffLineParse 
    (
    int 	argc,
    char *	argv[]
    )
    {
    extern char *	optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    resetBuff = 0;
    destroyBuff = 0;
    verbose = 0;
    tgtCheck = 0;
    
    while ((ch = getopt (argc, argv, "dhrtv")) != -1)
    	{
	switch (ch)
	    {
	    case 'd':
	    	destroyBuff = 1; break;
	    case 'r':
	    	resetBuff = 1; break;
	    case 'v':
	    	verbose = 1; break;
	    case 't':
	    	tgtCheck = 1; break;
	    case 'h':
	    default:
	    	wvOffUsage (); return (1);
	    }
	}
    return (0);
    }

/*****************************************************************************
*
* wvOnLineParse - parse 'on' command options
*
* RETURNS: 0 or 1 if unsupported option is found
*
*/

int wvOnLineParse 
    (
    int 	argc,
    char *	argv[]
    )
    {
    extern char *	optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    wvUploadPathArg1 =
    wvUploadPathArg2 = 0;
    upCntMax = 100;

    while ((ch = getopt (argc, argv, "dig:hl:o:n:p:P:F:f:S:s:")) != -1)
    	{
	switch (ch)
	    {
	    WV_INST_LVL_OPTION_PARSE(wvOnUsage)
	    case 'n': /* temporary upload number */
	    	upCntMax = strtoul (optarg, NULL, 0);
		if (upCntMax < 0 || upCntMax > 100)
		    upCntMax = 100;
		break;
	    WV_UPLOAD_OPTION_PARSE(wvOnUsage);
	    case 'h':
	    default:
	    	wvOnUsage(); return (1);
	    }
	}

    /* since no default host is provided check one is provided */

    if ((_func_wvOnUploadPathCreate == (FUNCPTR)sockUploadPathCreate) &&
    	(wvUploadPathArg1 == 0))
	{
	printf ("SOCKET upload path requires a host (-S xx.xx.xx.xx)\n");
	return (1);
	}
    return (0);
    }

/*****************************************************************************
*
* wvUploadLineParse - parse 'upload' command options
*
* RETURNS: 0 or 1 if unsupported option is found
*
*/

int wvUploadLineParse 
    (
    int 	argc,
    char *	argv[]
    )
    {
    extern char *	optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    wvUploadPathArg1 =
    wvUploadPathArg2 = 0;

    while ((ch = getopt (argc, argv, "htP:F:f:S:s:")) != -1)
    	{
	switch (ch)
	    {
	    WV_UPLOAD_OPTION_PARSE(wvUploadUsage);
	
	    case't':	/* test only verify upload path creation */
	    	uploadTest = 1;
		break;

	    case 'h':
	    default:
	    	wvUploadUsage(); return (1);
	    }
	}
    return (0);
    }

/*****************************************************************************
*
* wvCfg - perform event buffer configuration
*
* RETURNS: 0 or 1 if an error occurs
*
*/

int wvCfg ()
    {
    int wrLogFd;

    WV_EVTLOG_OPEN(wrLogFd, O_WRONLY)

    /* set buffer configuration */

    if (ioctl (wrLogFd, DHIOCCONFIG, &rBuffCreateParams) != 0)
    	{
	perror ("Can't set configuration\n");
	WV_EVTLOG_CLOSE_AND_RETURN (wrLogFd, 1);
	}

    WV_EVTLOG_CLASSSET(wrLogFd, wvInst)

    WV_EVTLOG_CLOSE_AND_RETURN (wrLogFd, 0);
    }

/*****************************************************************************
*
* wvOff - stop event logging
*
* RETURNS: 0 or 1 if an error occurs
*
*/

int wvOff ()
    {
    int			wrLogFd;
    int			tgt;
    rbuff_info_t	rBuffInfo;
    int			retVal = 0;

    WV_EVTLOG_OPEN (wrLogFd, O_WRONLY);

    /* stop event collection */
    memset (&rBuffInfo, 0, sizeof(rbuff_info_t));
    if (ioctl (wrLogFd, DHIOCLOGOFF, &rBuffInfo) != 0)
    	{
	perror ("Can't turn off instrumentation\n");
	WV_EVTLOG_CLOSE_AND_RETURN (wrLogFd, 1);
	}

    if (verbose)
    	{
	printf ("Buffer statistics:\n");
	printf ("\toptions %#x\n", rBuffInfo.options);
	printf ("\tminimum buffer number: %d\n", rBuffInfo.minBuffs);
	printf ("\tmaximum buffer number: %d\n", rBuffInfo.maxBuffs);
	printf ("\tbuffer size: 0x%x\n", rBuffInfo.buffSize);
	printf ("\tmaximum used buffer number: %d\n", rBuffInfo.maxBuffsActual);
	printf ("\tcurrent buffer number: %d\n", rBuffInfo.currBuffs);
	printf ("\tdata content: %d bytes\n", rBuffInfo.dataContent);
	printf ("\ttimes extended: %d\n", rBuffInfo.timesExtended);
	}
    
    if (tgtCheck && ((tgt = ioctl (wrLogFd, DHIOCTGTCOLLECTED, 0)) != 0))
	{
	perror ("target event collected\n");
	retVal = tgt;
	}

    if (resetBuff && (ioctl (wrLogFd, DHIOCRESET, 0) != 0))
	{
	perror ("Can't reset event buffer\n");
	WV_EVTLOG_CLOSE_AND_RETURN (wrLogFd, 1);
	}

    if (destroyBuff && (ioctl (wrLogFd, DHIOCRBUFFDESTROY, 0) != 0))
    	{
	perror ("Can't destroy event buffer\n");
	WV_EVTLOG_CLOSE_AND_RETURN (wrLogFd, 1);
	}

    WV_EVTLOG_CLOSE_AND_RETURN (wrLogFd, retVal);
    }

/*****************************************************************************
*
* wvOn - start event logging 
*
* RETURNS: 0 or 1 if an error occurs
*
*/

int wvOn ()
    {
    int			wrLogFd;
    rbuff_info_t	rBuffInfo;
    int			upCnt;
    fd_set		bufReady;
    pid_t		uploadPid;

    WV_EVTLOG_OPEN(wrLogFd, O_RDWR)	/* open logging device */

    memset (&rBuffInfo, 0, sizeof(rbuff_info_t));
    if (ioctl (wrLogFd, DHIOCRBUFFINFO, &rBuffInfo) != 0)
    	{
	perror ("Failed to get buffer configuration\n");
	WV_EVTLOG_CLOSE_AND_RETURN(wrLogFd, 1);
	}

    if (setWvInst)
	{
	/*
	 * if we change instrumentation level we must reset buffer,
	 * reset must be done before an upload process started
	 */

	if (ioctl (wrLogFd, DHIOCRESET, 0) != 0)
	    {
	    perror ("Can't reset buffer\n");
	    WV_EVTLOG_CLOSE_AND_RETURN (wrLogFd, 1);
	    }
	}

    /*
     * if continuous upload is selected start a process that will wait for
     * a full buffer to upload. The process is started before CLASSSET
     * request so that it is correctly added in existing process list logged
     * at event collection beginning.
     */

    if ((rBuffInfo.options & RBUFF_UP_DEFERRED) == 0)
    	{
	/*
	 * if there is no upload process, start a new one.
	 * If one exists ioctl will fail and return EBUSY.
	 */

	/* if none already set default path is FILE(/tmp/eventLog.wvr) */
    
	WV_DEFAULT_UPLOAD_PATH_SET

	WV_UPLOAD_PATH_CREATE(wrLogFd)	/* upload path creation */

	/* create upload process */

	if ((uploadPid = fork ()) < 0)
	    {
	    perror ("Failed to create upload process");
	    WV_UPLOAD_PATH_CLOSE		/* close upload path */
	    WV_EVTLOG_CLOSE_AND_RETURN(wrLogFd,1);
	    }

	if (uploadPid != 0)	/* parent can now start instrumentation */
	    {
	    WV_UPLOAD_PATH_CLOSE		/* close upload path */
	    }
	else			/* child is ready to upload */
	    {
	    if (ioctl (wrLogFd, DHIOCLOCK, 0) == 0)
		{
		upCnt = 0;
		while (upCnt <= upCntMax)
		    {
		    /* printf ("upload #%d\n", upCnt++); */
		    WV_CONTINUOUS_UPLOAD(wrLogFd, bufReady, rBuffInfo);
		    }

		}
	    close (wrLogFd);

	    WV_UPLOAD_PATH_CLOSE		/* close upload path */
	    exit (0);
	    }
	}

    WV_EVTLOG_CLASSSET(wrLogFd, wvInst)	/* possibly set logging level */
	
    /* turn on event logging */

    if (ioctl (wrLogFd, DHIOCLOGON, &rBuffInfo) != 0)
    	{
	perror ("Can't turn on instrumentation\n");
	WV_EVTLOG_CLOSE_AND_RETURN (wrLogFd, 1);
	}
    close (wrLogFd);

    return (0);
    }

/*****************************************************************************
*
* wvUploadCmd - perform event buffer upload
*
* RETURNS: 0 or 1 if an error occurs
*
*/

int wvUploadCmd ()
    {
    int			wrLogFd;
    rbuff_info_t	rBuffInfo;
    fd_set		bufReady;
    pid_t		uploadPid;

    WV_EVTLOG_OPEN (wrLogFd, O_RDWR)
    
    /* retrieve rbuff configuration */

    memset (&rBuffInfo, 0, sizeof(rbuff_info_t));
    if (ioctl (wrLogFd, DHIOCRBUFFINFO, &rBuffInfo) < 0)
    	{
	perror ("Can't retrieve evtlog buffer information\n");
	WV_EVTLOG_CLOSE_AND_RETURN(wrLogFd, 1);
	}
    
    /* if none already set default upload path is FILE(/tmp/eventLog.evr) */

    WV_DEFAULT_UPLOAD_PATH_SET	
    
    WV_UPLOAD_PATH_CREATE(wrLogFd)		/* upload path creation */

    /*
     * return 0 if we are only testing upload path creation
     * failure would already have cause to return 1
     */
    if (uploadTest)	
    	{
	WV_UPLOAD_PATH_CLOSE
	WV_EVTLOG_CLOSE_AND_RETURN(wrLogFd, 0);
	}

    /* Either wait for a buffer to upload or upload the entire buffer */

    if ((rBuffInfo.options & RBUFF_UP_DEFERRED) == 0)
	{
	/* continuous upload :
	 * start an upload process and return OK 
	 */
	if ((uploadPid = fork()) < 0)
	    {
	    perror ("Failed to create upload process");
	    WV_UPLOAD_PATH_CLOSE
	    WV_EVTLOG_CLOSE_AND_RETURN(wrLogFd, 1);
	    }

	if (uploadPid != 0)	/* parent can now start instrumentation */
	    {
	    WV_UPLOAD_PATH_CLOSE		/* close upload path */
	    WV_EVTLOG_CLOSE_AND_RETURN(wrLogFd, 0);
	    }
	else			/* child is ready to upload */
	    {
	    if (ioctl (wrLogFd, DHIOCLOCK, 0) != 0)
		{
		perror ("Can't lock buffer for uploading\n");
		WV_UPLOAD_PATH_CLOSE
		WV_EVTLOG_CLOSE_AND_RETURN(wrLogFd, 1);
		}

	    while (1)
		{
		WV_CONTINUOUS_UPLOAD(wrLogFd, bufReady, rBuffInfo);
		}
	    }
	}
    else
	{
	if (ioctl (wrLogFd, DHIOCLOCK, 0) != 0)
	    {
	    perror ("Can't lock buffer for uploading\n");
	    WV_EVTLOG_CLOSE_AND_RETURN(wrLogFd, 1);
	    }
	wvUpload (wrLogFd, wvUploadPathId, 0, 1, &rBuffInfo);
	}

    WV_UPLOAD_PATH_CLOSE

    WV_EVTLOG_CLOSE_AND_RETURN (wrLogFd, 0);
    }

/*****************************************************************************
*
* wvUsage - print supported commands
*
* RETURNS: N/A
*
*/

void wvUsage ()
    {
    printf ("wv usage:\n");
    printf ("\twv [cfg|off|on|upload] [cmd options]\n");
    }

/*****************************************************************************
*
* wvCfgUsage - print 'config' supported options
*
* RETURNS: N/A
*
*/

void wvCfgUsage (void)
    {
    printf ("'wv cfg' options:\n");
    printf ("\t[-h] prints this message\n");
    printf ("\t[-D] deferred upload\n");
    printf ("\t[-w] wrap-around when buffer is full\n");
    printf ("\t[-m min] set minimal number of buffer\n");
    printf ("\t[-M max] set maximal number of buffer\n");
    printf ("\t[-s size] set buffer size\n");
    printf ("\t[-l level] set instrumentation level (1, 3, 7)\n");
    printf ("\t[-o inst] additionnal instrumentation\n");
    printf ("\t[-p pid | -g pgid]\n");
    printf ("\t      restrict instrumentation to process/process group\n");
    printf ("\t[-d] descend instrumentation level on children too\n");
    printf ("\t[-i] future children inherit instrumentation level\n");
    }

/*****************************************************************************
*
* wvOffUsage - print 'off' command supported options
*
* RETURNS: N/A
*
*/

void wvOffUsage (void)
    {
    printf ("'wv off' options:\n");
    printf ("\t[-h] print this message\n");
    printf ("\t[-d] destroy buffer\n");
    printf ("\t[-r] reset buffer (erase buffer content)\n");
    printf ("\t[-v] display buffer status\n");
    }

/*****************************************************************************
*
* wvOnUsage - print 'on' command supported options
*
* RETURNS: N/A
*
*/

void wvOnUsage (void)
    {
    printf ("'wv on' options:\n");
    printf ("\t[-h] print this message\n");
    printf ("\t[-l level] set instrumentation level (1, 3, 7)\n");
    printf ("\t[-o inst] additionnal instrumentation\n");
    printf ("\t[-p pid | -g pgid]\n");
    printf ("\t      restrict instrumentation to process/process group\n");
    printf ("\t[-d] descend instrumentation level on children too\n");
    printf ("\t[-i] future children inherit instrumentation level\n");
    printf ("\t[-P FILE|SOCKET] upload path\n");
    printf ("\t[-F filename [-f openflag]] FILE upload path informations\n");
    printf ("\t[-S hostIp [-s portNb]] SOCKET upload path informations\n");
    }

/*****************************************************************************
*
* wvUploadUsage - print 'upload' command supported options
*
* RETURNS: N/A
*
*/

void wvUploadUsage (void)
    {
    printf ("'wv upload' options:\n");
    printf ("\t[-h] print this message\n");
    printf ("\t[-P FILE|SOCKET] upload path\n");
    printf ("\t[-F filename [-f openflag]] FILE upload path informations\n");
    printf ("\t[-S hostIp [-s portNb]] SOCKET upload path informations\n");
    }
