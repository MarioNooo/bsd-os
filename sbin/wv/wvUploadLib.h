/* wvUploadLib.h - library header file */

#ifndef __INCwvUploadLibh
#define __INCwvUploadLibh

#include "wvUploadPathP.h"

/* function declarations */

extern void		uploadSigHandler (int signo);
extern STATUS		uploadPathWrite (UPLOAD_ID pathId, char * pData,
					 int nBytes);
extern int		wvUpload (int logFd, UPLOAD_ID pathId, int nToUpload, 
				  int drainBuffer, 
				  rbuff_info_t * pRBuffInfo);

#endif /* __INCwvUploadLibh */
