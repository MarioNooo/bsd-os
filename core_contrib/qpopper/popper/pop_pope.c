#include "config.h"
#include <stdio.h>
#include <sys/types.h>


#include <pwd.h>
#include "popper.h"


int pop_epop(p)
POP * p;

{
    fputs("+OK\r\n", p->output);
    fputs("TOP\r\n", p->output) ;
    fputs("PIPELINING\r\n", p->output) ;
#ifndef SCRAM_ONLY
# ifdef APOP
    fputs("APOP\r\n", p->output) ;
# endif
#endif
#ifdef SCRAM
    fputs("AUTH SCRAM-MD5\r\n", p->output) ;
#endif
#ifndef SCRAM_ONLY
# ifndef APOP_ONLY
    fputs("USER\r\n", p->output);
# endif
#endif
    fputs("UIDL\r\n", p->output);
    fputs("MANGLE\r\n", p->output);
    fputs(".\r\n", p->output);
    fflush(p->output);
    return(POP_SUCCESS);
}
