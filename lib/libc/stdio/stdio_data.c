/* BSDI stdio_data.c,v 2.3 1996/11/08 06:48:08 torek Exp */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "local.h"
#include "glue.h"

#define	std(flags, file, filestruct) \
	{0,0,0,flags,file,{0},0,filestruct,__sclose,__sread,__sseek,__swrite}
/*	 p r w flags file _bf z  cookie       close    read    seek    write */

				/* the usual - (stdin + stdout + stderr) */
static FILE usual[FOPEN_MAX - 3];

FILE __sstdin = std(__SRD, STDIN_FILENO, &__sstdin);
FILE __sstdout = std(__SWR, STDOUT_FILENO, &__sstdout);
FILE __sstderr = std(__SWR|__SNBF, STDERR_FILENO, &__sstderr);

static struct glue uglue = { 0, FOPEN_MAX - 3, usual };
static struct glue glueerr = { &uglue, 1, &__sstderr };
static struct glue glueout = { &glueerr, 1, &__sstdout };
struct glue __sglue = { &glueout, 1, &__sstdin };

pthread_mutex_t __sglobal_lock = PTHREAD_MUTEX_INITIALIZER;
