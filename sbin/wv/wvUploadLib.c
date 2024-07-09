/* wvUploadLib.c - upload event log */

/* includes */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>

#include <sys/evtlog.h>
#include "wvUploadPathP.h"
#include "wvFileUploadPathLibP.h"
#include "wvSockUploadPathLibP.h"

/* defines */

#define EVT_LOG_DEV		"/dev/evtlog"

#define WV_DFLT_FILE		"/tmp/wvLog.wvr"
#define WV_DFLT_FLAG		(O_CREAT | O_TRUNC)	
#define WV_DFLT_PORT		0	/* XXX ELP to be changed */

/* locals */

static int	wvUploadMaxAttempts = 200;	/* upload write retries */
static int	wvUploadRetryBackoff = 100000;	/* upload retry delay (us) */

/*******************************************************************************
*
* uploadPathWrite - write data to upload path
*
* This routine attempts to write the data pointed to by pData to the uplaod
* path indicated by pathId.  If the data can not be written due to a blocking
* error such as EWOULDBLOCK or EAGAIN, this routine will delay for some time
* using taskDelay(), and retry until the data is written.  If the data can
* not be written within a reasonable number of attempts, or an error other
* than EWOULDBLOCK or EAGAIN is generated while writing, this routine will
* give up and return an error.
*
* RETURNS: OK, or ERROR if the data could not be written in a reasonable
*          number of attempts, or if invalid pathId
*
* SEE ALSO:
* NOMANUAL
*
*/

static STATUS uploadPathWrite
    (
    UPLOAD_ID	pathId,		/* path to write to */
    char *	pData,		/* buffer of data to write */
    int		nBytes		/* number bytes to write */
    )
    {
    int			trys;		/* to count the attempts */
    int			nToWrite;	/* number bytes left to write  */
    int			nThisTry;	/* number bytes written this try */
    int			nWritten;	/* total bytes written so far */
    char *		buf;		/* local copy of pData */

    trys     = 0;
    nWritten = 0;
    nThisTry = 0;
    nToWrite = nBytes;
    buf      = pData;

    if (pathId == NULL || pathId->writeRtn == NULL)
        {
	printf ("pathId = %p, pathId->writeRtn = %p\n",
		pathId, pathId->writeRtn);
	return (ERROR);
	}

    while (nWritten < nBytes && trys < wvUploadMaxAttempts)
        {
        if ((nThisTry = pathId->writeRtn (pathId, buf, nToWrite)) < 0)
            {

            /* If write failed because it would have blocked, allow retry. */

            if (errno == EWOULDBLOCK || errno == EAGAIN)
                usleep (wvUploadRetryBackoff);
            else
                return (ERROR);
            }
        else
            {
            nWritten += nThisTry;
            nToWrite -= nThisTry;
            buf      += nThisTry;
            }

        ++trys;
        }

    /* Check if write failed to complete within the max number of tries. */

    if (nWritten < nBytes)
        return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* wvUpload -
*
*/

int wvUpload 
    (
    int			logFd,
    UPLOAD_ID		wvUploadPathId,
    int			nToUpload,
    int			drainBuffer,
    rbuff_info_t *	pRBuffInfo
    )
    {
    int		nToRead = nToUpload;
    int		wholeBuffer = (nToUpload == 0);
    int		nUploaded = 0;
    int		nToWrite;
    rbuff_ptr_t	pBuf;
#if 1
    char *	addr;
    int 	ix;
#endif

    while ((nToRead > 0) || (wholeBuffer))
    	{
	/* printf ("left to read = %d\n", nToRead); */
	if ((pBuf = mmap (NULL,
			  pRBuffInfo->buffSize,
			  PROT_READ,
			  MAP_SHARED,
			  logFd,
			  0)) == MAP_FAILED)
	    {
	    printf ("Can't read event buffer\n");
	    return 1;
	    }

#if 0
	printf ("buffer mapped successfully at %p(%d) offset %d\n",
		pBuf, pBuf->dataLen, pBuf->readOffset);

	addr = (char *)pBuf+sizeof(rbuff_buff_t)+pBuf->readOffset;
	for (ix = 0; ix < 1; ix++)
	     printf ("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	     	     *(addr + ix), *(addr + ix + 1), *(addr + ix + 2),
		     *(addr + ix + 3), *(addr + ix + 4), *(addr + ix + 5),
		     *(addr + ix + 6), *(addr + ix + 7), *(addr + ix + 8),
		     *(addr + ix + 9));
	sleep (3);
#endif

	if (wholeBuffer)
	    {
	    nToWrite = nToRead = pBuf->dataLen;
	    }
	else
	    {
	    nToWrite = MIN (pBuf->dataLen , (nToUpload - nUploaded)); 
	    }

	if (nToWrite == 0)	/* buffer is empty */
	    {
	    /* printf ("buffer is empty\n"); */
	    return 1;
	    }
	
#if 0
	if (drainBuffer)
	    printf ("draining %d\n", nToWrite);
#endif

	nToRead -= nToWrite;

	/* use upload path to upload the data */

	if (wvUploadPathId != NULL)
	    {
#if 0
	    printf ("writing %d from %p\n", nToWrite,
	    	    ((char *)pBuf+sizeof(rbuff_buff_t)+pBuf->readOffset));
#endif
	    if (uploadPathWrite (wvUploadPathId,
	    			 ((char *) pBuf + sizeof(rbuff_buff_t) +
				  pBuf->readOffset),
				 nToWrite)
	    	== ERROR)
	    	{
		perror ("Can't upload\n");
		return 1;
		}
	    }

	nUploaded += nToWrite;
	if (ioctl (logFd, DHIOCREADCOMMIT, &nToWrite) != 0)
	    {
	    printf ("Can't commit %d\n", nToWrite);
	    return 1;
	    }

	munmap (pBuf, pRBuffInfo->buffSize);

	if (! drainBuffer)
	    wholeBuffer = 0;
	}
    return 0;
    }
