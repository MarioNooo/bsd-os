/**************************************************************************
 * LPRng IFHP Filter
 * Copyright 1994-1999 Patrick Powell, San Diego, CA <papowell@astart.com>
 **************************************************************************/
/**** HEADER *****/
static char *const _id = "$Id: vars.c,v 1.13 2000/04/13 10:56:31 papowell Exp papowell $";

#define EXTERN
#include "ifhp.h"

/**** ENDINCLUDE ****/

struct keyvalue Valuelist[] = {

    {  "Accounting_script", "accounting", &Accounting_script, STRV  },
    {  "Appsocket", "appsocket", (char **)&Appsocket, FLGV  },
    {  "Autodetect", "autodetect", (char **)&Autodetect, FLGV },
    {  "Crlf", "crlf", (char **)&Crlf, FLGV },
    {  "Dev_retries", "dev_retries", (char **)&Dev_retries, INTV },
    {  "Dev_sleep", "dev_sleep", (char **)&Dev_sleep, INTV },
    {  "Device", "dev", &Device, STRV },
    {  "End_ctrl_t", "end_ctrl_t", &End_ctrl_t, STRV },
    {  "Force_status", "forcestatus", (char **)&Force_status, FLGV },
    {  "Force_conversion", "forceconversion", (char **)&Force_conversion, FLGV },
    {  "Full_time", "fulltime", (char **)&Full_time, FLGV },
    {  "Initial_timeout", "initial_timeout", (char **)&Initial_timeout, INTV },
    {  "Ignore_eof", "ignore_eof", (char **)&Ignore_eof, FLGV },
    {  "Job_timeout", "job_timeout", (char **)&Job_timeout, INTV },
    {  "Logall", "logall", (char **)&Logall, FLGV },
    {  "Max_status_size", "statusfile_max", (char **)&Max_status_size, INTV },
    {  "Min_status_size", "statusfile_min", (char **)&Min_status_size, INTV },
    {  "Null_pad_count", "nullpad", (char **)&Null_pad_count, INTV },
    {  "No_PCL_EOJ", "no_pcl_eoj", (char **)&No_PCL_EOJ, FLGV },
    {  "No_PS_EOJ", "no_ps_eoj", (char **)&No_PS_EOJ, FLGV },
    {  "No_udp_monitor", "no_udp_monitor", (char **)&No_udp_monitor, FLGV },
    {  "Pagecount", "pagecount", &Pagecount, STRV },
    {  "Pagecount_interval", "pagecount_interval", (char **)&Pagecount_interval, INTV },
    {  "Pagecount_poll", "pagecount_poll", (char **)&Pagecount_poll, INTV },
    {  "Pagecount_timeout", "pagecount_timeout", (char **)&Pagecount_timeout, INTV },
    {  "Pcl", "pcl", (char **)&Pcl, FLGV },
    {  "Pjl", "pjl", (char **)&Pjl, FLGV },
    {  "Pjl_console", "pjl_console", (char **)&Pjl_console, FLGV },
    {  "Pjl_enter", "pjl_enter", (char **)&Pjl_enter, FLGV },
    {  "Ps", "ps", (char **)&Ps, FLGV },
    {  "Ps_pagecount_code", "ps_pagecount_code", &Ps_pagecount_code, STRV },
    {  "Ps_status_code", "ps_status_code", &Ps_status_code, STRV },
    {  "Psonly", "psonly", (char **)&Psonly, FLGV },
    {  "Qms", "qms", (char **)&Qms, FLGV  },
    {  "Read_write", "read_write", (char **)&Read_write, FLGV  },
    {  "Remove_ctrl", "remove_ctrl", &Remove_ctrl, STRV  },
    {  "Status", "status", (char **)&Status, FLGV },
    {  "Stty_args", "stty", &Stty_args, STRV },
    {  "Sync", "sync", &Sync, STRV },
    {  "Sync_interval", "sync_interval", (char **)&Sync_interval, INTV },
    {  "Sync_timeout", "sync_timeout", (char **)&Sync_timeout, INTV },
    {  "Tbcp", "tbcp", (char **)&Tbcp, FLGV },
    {  "Text", "text", (char **)&Text, FLGV },
    {  "Trace_on_stderr", "trace", (char **)&Trace_on_stderr, FLGV },
    {  "Waitend", "waitend", &Waitend, STRV },
    {  "Waitend_ctrl_t_interval", "waitend_ctrl_t_interval", (char **)&Waitend_ctrl_t_interval, INTV },
    {  "Waitend_interval", "waitend_interval", (char **)&Waitend_interval, INTV },

    { 0, 0, 0 }
};
