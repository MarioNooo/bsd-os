/* trg.c - trigger management command */

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

/* defines */

#define EVT_LOG_DEV	"/dev/evtlog"

#define TRG_EVTLOG_OPEN(logFd)				\
    if ((logFd = open (EVT_LOG_DEV, O_WRONLY)) < 0)	\
    	{						\
	perror ("Can't open event logging device ("EVT_LOG_DEV"),\nmake sure evtlog pseudo-device is configured and SMP option is not.\n");  \
	return (1);					\
	}

#define TRG_EVTLOG_CLOSE_AND_RETURN(logFd, retVal)	\
    close (logFd);					\
    return (retVal);

#define TRG_ACTION(actionType)				\
    ((actionType == TRG_ACT_NONE) ? "NONE" : 		\
     ((actionType == TRG_ACT_WV_START) ? "WV_START" : "WV_STOP"))

#define TRG_CTX_TYPE(ctxType)				\
    ((ctxType == TRG_CTX_ANY) ? "ANY" :			\
     ((ctxType == TRG_CTX_ANY_TASK) ? "ANY_TASK" :	\
      ((ctxType == TRG_CTX_TASK) ? "TASK" :		\
       ((ctxType == TRG_CTX_ANY_ISR) ? "ISR" :		\
        ((ctxType == TRG_CTX_ISR) ? "ANY_ISR" :		\
	 "SYSTEM")))))

/* locals */

static int	triggerCmd;
static char *	triggerArg;

static trigger_t	trigger;
static int		trgListAll;
static int		trgDelAll;

/* forward declarations */

int	trgCmdLineParse (int argc, char * argv[]);
int	trgAddLineParse (int argc, char * argv[]);
int	trgChainLineParse (int argc, char * argv[]);
int	trgDeleteLineParse (int argc, char * argv[]);
int	trgStateUpdateLineParse (int argc, char * argv[]);
int	trgListLineParse (int argc, char * argv[]);

int	trgCmd (void);
int	trgDeleteCmd (void);
int	trgListCmd (void);

void	trgCmdUsage (void);
void	trgAddUsage (void);
void	trgChainUsage (void);
void	trgDeleteUsage (void);
void	trgStateUpdateUsage (void);
void	trgListUsage (void);

/*****************************************************************************
*
* main - entry point
*
*/

int main (argc, argv)
    int		argc;
    char *	argv[];
    {
    return (trgCmdLineParse (argc, argv));
    }

/******************************************************************************
*
* trgCmdLineParse - 
*
*/

int trgCmdLineParse
    (
    int		argc,
    char *	argv[]
    )
    {
    extern char *	optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    /* get command first no command assumes 'on' */

    if (argc > 1)
    	{
	if (strcmp (argv[1], "add") == 0)
	    {
	    if (trgAddLineParse (--argc, ++argv))
	    	return (1);
	    return (trgAddCmd ());
	    }
	else if (strcmp (argv[1], "chain") == 0)
	    {
	    if (trgChainLineParse (--argc, ++argv))
	    	return (1);
	    triggerCmd = DHIOCTRGUPDATE;
	    }
	else if (strcmp (argv[1], "del") == 0)
	    {
	    if (trgDeleteLineParse (--argc, ++argv))
	    	return (1);
	    return (trgDeleteCmd ());
	    }
	else if (strcmp (argv[1], "dis") == 0)
	    {
	    if (trgStateUpdateLineParse (--argc, ++argv))
	    	return (1);
	    trigger.status = TRG_DISABLE;
	    triggerCmd = DHIOCTRGUPDATE;
	    }
	else if (strcmp (argv[1], "en") == 0)
	    {
	    if (trgStateUpdateLineParse (--argc, ++argv))
	    	return (1);
	    trigger.status = TRG_ENABLE;
	    triggerCmd = DHIOCTRGUPDATE;
	    }
	else if (strcmp (argv[1], "list") == 0)
	    {
	    if (trgListLineParse (--argc, ++argv))
	    	return (1);
	    return (trgListCmd ());
	    }
	else if (strcmp (argv[1], "on") == 0)
	    {
	    triggerCmd = DHIOCTRGON;
	    }
	else if (strcmp (argv[1], "off") == 0)
	    {
	    triggerCmd = DHIOCTRGOFF;
	    }
	else
	    {
	    trgCmdUsage ();
	    return (1);
	    }
	}
    else
    	triggerCmd = DHIOCTRGON;

    return (trgCmd ());
    }

/*****************************************************************************
*
* trgAddLineParse - parse trigger addition options
*
* returns: 0 or 1 if an invalid option is found
*
*/

int trgAddLineParse 
    (
    int		argc,
    char *	argv[]
    )
    {
    extern char *       optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    bzero (trigger, sizeof (trigger_t));
    trigger.eventId = EVENT_ANY_EVENT;

    while ((ch = getopt (argc, argv, "Ehsa:c:e:p:t:")) != -1)
    	{
	switch (ch)
	    {
	    case 'E':
	    	trigger.status = TRG_ENABLE;
		break;
	    case 's':
		trigger.disable = 1;
		break;
	    case 'a':
	    	trigger.actionType = strtoul (optarg, NULL, 0);
		if (trigger.actionType != TRG_ACT_NONE &&
		    trigger.actionType != TRG_ACT_WV_START &&
		    trigger.actionType != TRG_ACT_WV_STOP)
		    {
		    errno = EINVAL;
		    perror ("Unsupported action type");
		    return 1;
		    }
		break;
	    case 'c':
	    	trigger.chain = (trigger_t *)strtoul (optarg, NULL, 0);
		break;
	    case 'e':
	    	trigger.eventId = strtoul (optarg, NULL, 0);
		break;
	    case 'p':
	    	trigger.contextId = strtoul (optarg, NULL, 0);
		break;
	    case 't':
	    	trigger.contextType = strtoul (optarg, NULL, 0);
		if (trigger.contextType < TRG_CTX_ANY ||
		    trigger.contextType > TRG_CTX_ISR)
		    {
		    errno = EINVAL;
		    perror ("Unsupported context type");
		    return (1);
		    }
		break;
	    case 'h':
	    default:
	    	trgAddUsage ();
		return (1);
	    }
	}
    triggerArg = (char *)&trigger;
    return (0);
    }

/*****************************************************************************
*
* trgChainLineParse - parse chain command option
*
* RETURNS: 0 or 1 if unsupported option is found
*
*/

int trgChainLineParse
    (
    int		argc,
    char *	argv[]
    )
    {
    extern char *       optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    bzero (trigger, sizeof (trigger_t));

    while ((ch = getopt (argc, argv, "hc:i:")) != -1)
    	{
	switch (ch)
	    {
	    case 'c':	/* read chain */
		trigger.chain = (trigger_id_t)strtoul (optarg, NULL, 0);
		break;
	    case 'i':	/* trigger to update */
		trigger.triggerId = (trigger_id_t)strtoul (optarg, NULL, 0);
		break;
	    case 'h':
	    default:
		trgChainUsage ();
		return (1);
	    }
	}
    
    /* triggerId and chain are required */

    if (trigger.triggerId == 0 || trigger.chain == 0)	
    	{
	perror ("-i trgId -c chainId options are mandatory\n");
	return (1);
	}

    triggerArg = (char *)&trigger;
    return (0);
    }

/*****************************************************************************
*
* trgDeleteLineParse - parse delete command options
*
* RETURNS: 0 or 1 if unsupported option is found
*
*/

int trgDeleteLineParse
    (
    int		argc,
    char *	argv[]
    )
    {
    extern char *       optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    bzero (trigger, sizeof (trigger_t));

    while ((ch = getopt (argc, argv, "ahi:")) != -1)
    	{
	switch (ch)
	    {
	    case 'a':
	    	trgDelAll = 1; break;
	    case 'i':
	    	trigger.triggerId = (trigger_id_t)strtoul (optarg, NULL, 0);
		break;
	    case 'h':
	    default:
	    	trgDeleteUsage ();
		return (1);
	    }
	}
    return (0);
    }

/*****************************************************************************
*
* trgStateUpdateLineParse - parse enable/disable command option
*
* RETURNS: 0 or 1 if unsupported option is found
*
*/

int trgStateUpdateLineParse
    (
    int		argc,
    char *	argv[]
    )
    {
    extern char *       optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    bzero (trigger, sizeof (trigger_t));

    while ((ch = getopt (argc, argv, "hi:")) != -1)
    	{
	switch (ch)
	    {
	    case 'i':	/* trigger to update */
		trigger.triggerId = (trigger_id_t)strtoul (optarg, NULL, 0);
		break;
	    case 'h':
	    default:
		trgStateUpdateUsage ();
		return (1);
	    }
	}
    
    /* triggerId is required */

    if (trigger.triggerId == 0)	
    	{
	perror ("-i trgId option is mandatory\n");
	return (1);
	}

    triggerArg = (char *)&trigger;
    return (0);
    }

/*****************************************************************************
* 
* trgListParse - parse list command options
*
* RETURNS: 0 or 1 if unsupported option is found
*
*/

int trgListLineParse
    (
    int		argc,
    char *	argv[]
    )
    {
    extern char *       optarg;
    extern int		optind;
    extern int		optopt;
    extern int		opterr;
    extern int		optreset;
    int			ch;

    trgListAll = 1;	/* default option is to show all */
    bzero (trigger, sizeof (trigger_t));

    while ((ch = getopt (argc, argv, "ahi:")) != -1)
    	{
	switch (ch)
	    {
	    case 'a':
	    	trgListAll = 1; break;
	    case 'i':
	    	trgListAll = 0;
		trigger.triggerId = (trigger_id_t)strtoul (optarg, NULL, 0);
		break;
	    case 'h':
	    default:
	    	trgListUsage ();
		return (1);
	    }
	}
    return (0);
    }

/*****************************************************************************
*
* trgCmd - perform <triggerCmd>
*
* RETURNS: 0 or 1 if an error is detected
*
*/

int trgCmd ()
    {
    int rLogFd;

    TRG_EVTLOG_OPEN (rLogFd);		/* open event logging device */

    /* execute trigger command */

    if (ioctl (rLogFd, triggerCmd, triggerArg) != 0)
    	{
	perror ("trigger command failed\n");
	TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 1);
	}

    TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 0);
    }

/*****************************************************************************
*
* trgAddCmd - add trigger 
*
* RETURNS: triggerId or 0 
*/

int trgAddCmd ()
    {
    int rLogFd;
    int evtlogClass;

    TRG_EVTLOG_OPEN (rLogFd);		/* open event logging device */

    /* if a trigger starts/stops windview then windview must ne configured */

    if (trigger.actionType == TRG_ACT_WV_STOP ||
	trigger.actionType == TRG_ACT_WV_START)
	{
	if (ioctl (rLogFd, DHIOCEVTCLASSGET, evtlogClass) != 0)
	    {
	    perror ("trigger command failed\n");
	    TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 1);
	    }
	if (evtlogClass == 0)
	    {
	    perror ("windview logging should be configured first\n");
	    TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 1);
	    }
	}

    /* execute trigger command */

    if (ioctl (rLogFd, DHIOCTRGADD, &trigger) != 0)
    	{
	perror ("trigger command failed\n");
	TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 1);
	}

    printf ("0x%x\n", trigger.triggerId);
    TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 0);
    }

/*****************************************************************************
*
* trgDeleteCmd - delete trigger(s)
*
* This routine either delete a specific trigger or the whole trigger list
*
* RETURNS: 0 or 1 if an error occurs
*
*/

int trgDeleteCmd ()
    {
    int rLogFd;

    TRG_EVTLOG_OPEN (rLogFd);

    if (trgDelAll)
    	{
	bzero (&trigger, sizeof(trigger_t));

	while (ioctl (rLogFd, DHIOCTRGDELETE, &trigger) == 0)
	    {
	    if (trigger.next == NULL)
	    	{
		TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 0);
		}
	    trigger.triggerId = trigger.next;
	    }
	TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 1);
	}
    else
    	{
	if (ioctl (rLogFd, DHIOCTRGDELETE, &trigger) == 0)
	    {
	    perror ("Can't delete trigger\n");
	    TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 1);
	    }
	TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 0);
	}
    TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 0);
    }

/*****************************************************************************
*
* trgListCmd - retrieve trigger list
*
* This routine performs DHIOCTRGGET command (possibly in a loop to get every
* triggers)
*
* RETURNS: 0 or 1 if an error is detected
*
*/

#define TRG_HEADER_STRING 	\
    "triggerId | evtId | en | s | action  | ctxType | ctxId | chain\n"

int trgListCmd ()
    {
    int rLogFd;
    int once = 1;

    TRG_EVTLOG_OPEN (rLogFd);

    if (trgListAll)
    	{
	bzero (&trigger, sizeof(trigger_t));
	while (ioctl (rLogFd, DHIOCTRGGET, &trigger) == 0)
	    {
	    if (trigger.triggerId == NULL)	/* empty list */
	    	{
	    	TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 0);
		}
	    if (once)
	    	{
		printf ("Installed triggers:\n");
		printf (TRG_HEADER_STRING);
		/*       0x00000000  0    0   WV_START    ANY      0 */
		once = 0;
		}
	    printf ("0x%08x  %-5d   %d    %d   %-8s  %-8s    %d     0x%x\n",
	    	    trigger.triggerId, trigger.eventId, trigger.status,
		    trigger.disable, TRG_ACTION(trigger.actionType),
		    TRG_CTX_TYPE(trigger.contextType), trigger.contextId,
		    trigger.chain);

	    if (trigger.next == NULL)		/* end of list */
	    	{
	    	TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 0);
		}
	    trigger.triggerId = trigger.next;
	    }
	TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 1);
	}
    else	/* get information about a specific trigger */
    	{	
	if (ioctl (rLogFd, DHIOCTRGGET, &trigger) != 0)
	    {
	    perror ("Can't retrieve trigger information\n");
	    TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 1);
	    }
	printf ("triggerId | en | s | action  | ctxType | ctxId\n");
	printf ("0x%08x  %d    %d   %s   %s      %d\n",
		trigger.triggerId, trigger.status, trigger.disable,
		TRG_ACTION(trigger.actionType),
		TRG_CTX_TYPE(trigger.contextType),
		trigger.contextId);
	TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 0);
	}
    TRG_EVTLOG_CLOSE_AND_RETURN (rLogFd, 0);
    }

/*****************************************************************************
*
* trgAddUsage - Show 'add' command supported options
*
* returns: N/A
*/

void trgAddUsage ()
    {
    printf ("'trg add' supported options:\n");
    printf ("\t[-s] single usage: trigger is disabled as soon it is hit\n");
    printf ("\t[-E] enable trigger\n");
    printf ("\t[-a actionId] action when trigger is hit\n");
    printf ("\t\t0: no action\n");
    printf ("\t\t1: start windview event collection\n");
    printf ("\t\t2: stop windview event collection\n");
    printf ("\t[-c triggerId] chained trigger\n");
    printf ("\t[-e eventId] expected event\n");
    printf ("\t[-p pid] context id event should be received in\n");
    printf ("\t[-t ctxType] context type event should be received in\n");
    printf ("\t\t0: any context\n");
    printf ("\t\t1: any process\n");
    printf ("\t\t2: given process (-p option)\n");
    printf ("\t\t3: any interrupt thread\n");
    printf ("\t\t4: given interrupt thread (-p option)\n");
    printf ("\t\t5: system context\n");
    }

/*****************************************************************************
*
* trgChainUsage - show 'chain' trigger supported options
*
* RETURNS: N/A
*
*/

void trgChainUsage ()
    {
    printf ("trg chain\n");
    printf ("\t[-h] print this message\n");
    printf ("\t-c trgChainId trigger to be chained\n");
    printf ("\t-i trgId	     trigger to be updated\n");
    }

/*****************************************************************************
*
* trgDeleteUsage - print delete trigger supported options
*
* RETURNS: N/A
*
*/

void trgDeleteUsage ()
    {
    printf ("trg delete\n");
    printf ("\t[-h] print this message\n");
    printf ("\t[-a] delete every installed triggers\n");
    printf ("\t[-i triggerId] delete triggerId information\n");
    }

/*****************************************************************************
*
* trgStateUpdateUsage - print enable/disable trigger supported options
*
* RETURNS: N/A
*
*/

void trgStateUpdateUsage ()
    {
    printf ("trg enable|disable\n");
    printf ("\t[-h] print this message\n");
    printf ("\t-i trgId trigger to be updated\n");
    }

/*****************************************************************************
*
* trgListUsage - print list trigger supported options
*
* RETURNS: N/A
*
*/

void trgListUsage ()
    {
    printf ("trg list\n");
    printf ("\t[-h] print this message\n");
    printf ("\t[-a] list every installed triggers\n");
    printf ("\t[-i triggerId] display triggerId information\n");
    }

/*****************************************************************************
*
* trgCmdUsage -
*
*/

void trgCmdUsage (void)
    {
    printf ("trg usage:\n");
    printf ("\ttrg [add|chain|del|dis|en|list|on|off]\n");
    }
