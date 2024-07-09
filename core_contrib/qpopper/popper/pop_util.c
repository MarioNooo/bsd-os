/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * File: pop_util.c 
 *
 * Revisions: 
 *
 * 06/01/01  [rg]
 *          - Fix from Arvin Schnell for warnings on 64-bit systems.
 *
 * 04/02/01  [rg]
 *          - Changed macro STACKSIZE to QPSTACKSIZE to avoid conflicts.
 *
 * 03/09/00  [rg]
 *          - Changed type of StackInit from int to CALLSTACK *.
 *
 * 03/18/98  [py]
 */


#include "config.h"
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _pop POP;
#define QPSTACKSIZE 2                           /* Chosen for Implementation */
typedef void *(*FP)(POP *);
typedef struct CallStack {
    FP Stack[QPSTACKSIZE];                      /* Function Pointers array */
    int CurP;
} CALLSTACK;
/*
 * Simple stack Implementation
 * wish the compilers does inlining.
 */
FP Push(CALLSTACK *s, FP f)
{
    return ( s->CurP < QPSTACKSIZE ) ? ( s->Stack[s->CurP++] = f ) : (FP) 0;
}

FP Pop(CALLSTACK *s)
{
    return s->CurP ? s->Stack[--s->CurP] : (FP) 0;
}

int StackSize(CALLSTACK *s)
{
    return s->CurP;
}

CALLSTACK *StackInit(CALLSTACK *s)
{
    return (CALLSTACK *) memset ( s, 0, sizeof(CALLSTACK) );
}

FP GetTop(CALLSTACK *s, int *x)
{
    *x = s->CurP - 1;
    return (*x >= 0) ? s->Stack[(*x)] : (FP)0;
}

FP GetNext(CALLSTACK *s, int *x)
{
    (*x)--;
    return (*x >= 0) ? s->Stack[(*x)] : (FP)0;
}
static char *week_day[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};
char *get_time()       /* Returns the static rfc-822 format date string */
{
    struct tm *ltm, *TM;
    static char tzinfo[16]={0};
    static char rfc822_time[64];
    int tzoff;
    time_t now;
    time(&now);
    ltm = localtime(&now);
    TM = (struct tm *)malloc(sizeof(struct tm));
    memcpy(TM,ltm,sizeof(struct tm));
    ltm = TM;

    if(!tzinfo[0]) {
        struct tm *gtm;
        gtm = gmtime(&now);
        TM = (struct tm *)malloc(sizeof(struct tm));
        memcpy(TM,gtm,sizeof(struct tm));
        gtm = TM;

        tzoff = ((ltm->tm_min - gtm->tm_min) +
                 (60 * (ltm->tm_hour - gtm->tm_hour)) +
                 ((ltm->tm_year != gtm->tm_year) ?
                  ((ltm->tm_year > gtm->tm_year) ? 1440 : -1440) :
                  ((ltm->tm_yday != gtm->tm_yday) ?
                   ((ltm->tm_yday > gtm->tm_yday) ? 1440 : -1440) : 0)));
        tzinfo[0] = (tzoff >= 0) ? '+' : '-';
        if(tzinfo[0] == '-') tzoff = -tzoff;
        sprintf(&tzinfo[1],"%02d%02d",(tzoff/60),(tzoff%60));
        free(gtm);
    }

    (void) sprintf(rfc822_time, "%s, %d %s %04d %02d:%02d:%02d %s",
                   week_day[ltm->tm_wday], ltm->tm_mday,
                   months[ltm->tm_mon], ltm->tm_year + 1900,
                   ltm->tm_hour, ltm->tm_min, ltm->tm_sec,
                   tzinfo);
    free(ltm);
    return rfc822_time;
}
