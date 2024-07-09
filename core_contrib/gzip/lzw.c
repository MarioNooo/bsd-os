/* lzw.c -- compress files in LZW format.
 * This is a dummy version avoiding patent problems.
 */

#ifdef RCSID
static char rcsid[] = "lzw.c,v 1.1.1.2 1994/01/04 20:25:32 torek Exp";
#endif

#include "tailor.h"
#include "gzip.h"
#include "lzw.h"

static int msg_done = 0;

/* Compress in to out with lzw method. */
int lzw(in, out)
    int in, out;
{
    if (msg_done) return ERROR;
    msg_done = 1;
    fprintf(stderr,"output in compress .Z format not supported\n");
    if (in != out) { /* avoid warnings on unused variables */
        exit_code = ERROR;
    }
    return ERROR;
}
