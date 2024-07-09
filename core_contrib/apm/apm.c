/*
 * WILDBOAR $Wildboar: apm.c,v 1.4 1996/02/13 13:00:34 shigeya Exp $
 */
/*
 * 
 *  Portions or all of this file are Copyright(c) 1994,1995,1996
 *  Yoichi Shinoda, Yoshitaka Tokugawa, WIDE Project, Wildboar Project
 *  and Foretune.  All rights reserved.
 * 
 *  This code has been contributed to Berkeley Software Design, Inc.
 *  by the Wildboar Project and its contributors.
 *
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *      
 *  THIS SOFTWARE IS PROVIDED BY THE WILDBOAR PROJECT AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 *  WILDBOAR PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <machine/apmioctl.h>
#include <machine/apm.h>

/*
 * symbol table
 */
struct xlate {
  u_int value;		/* value	*/
  char *name;		/* name		*/
};

struct xlate no_args[] = {0,0};

struct xlate pm_events[] = {
  { APM_EVENT_STANDBY_REQ,	"APM_EVENT_STANDBY_REQ"		},
  { APM_EVENT_SUSPEND_REQ,	"APM_EVENT_SUSPEND_REQ"		},
  { APM_EVENT_NORMAL_RESUME,	"APM_EVENT_NORMAL_RESUME"	},
  { APM_EVENT_CRITICAL_RESUME,	"APM_EVENT_CRITICAL_RESUME"	},
  { APM_EVENT_LOW_BATTERY,	"APM_EVENT_LOW_BATTERY"		},
  { 0, 0 }
};

struct xlate pm_errors[] = {
  { APME_PM_DISABLED,		"APME_PM_DISABLED"		},
  { APME_CONN_BUSY,		"APME_CONN_BUSY"		},
  { APME_NO_CONN,		"APME_NO_CONN"			},
  { APME_REAL_NO_CONN,		"APME_REAL_NO_CONN"		},
  { APME_P16_CONN_BUSY,		"APME_P16_CONN_BUSY"		},
  { APME_P16_NOT_SUPPORTED,	"APME_P16_NOT_SUPPORTED"	},
  { APME_P32_CONN_BUSY,		"APME_P32_CONN_BUSY"		},
  { APME_P32_NOT_SUPPORTED,	"APME_P32_NOT_SUPPORTED"	},
  { APME_BAD_DEV,		"APME_BAD_DEV"			},
  { APME_INVAL,			"APME_INVAL"			},
  { APME_CANT_ENTER,		"APME_CANT_ENTER"		},
  { APME_NO_EVENT,		"APME_NO_EVENT"			},
  { APME_NOT_PRESENT,		"APME_NOT_PRESENT"		},
  { 0, 0 }
};

struct xlate devtab[] = {
  { APM_DEV_BIOS,		"bios"		},
  { APM_DEV_ALL,		"all"		},
  { APM_DEV_DISP_BASE,		"display"	},
  { APM_DEV_DISP_ALL,		"all_displays"	},
  { APM_DEV_DISK_BASE+0,	"disk0"		},
  { APM_DEV_DISK_BASE+1,	"disk1"		},
  { APM_DEV_DISK_ALL,		"all_disks"	},
  { APM_DEV_PPORT_BASE+0,	"pport0"	},
  { APM_DEV_PPORT_BASE+1,	"pport1"	},
  { APM_DEV_PPORT_ALL,		"all_pports"	},
  { APM_DEV_SPORT_BASE+0,	"sport0"	},
  { APM_DEV_SPORT_BASE+1,	"sport1"	},
  { APM_DEV_SPORT_ALL,		"all_sports"	},
  { 0xffff,			"all_fox"	}, /* XXX: what is this? */
  { 0, 0 },
};

struct xlate pstat_args[] = {
  { APM_PSTAT_READY,	"ready"		},
  { APM_PSTAT_STANDBY,	"standby"	},
  { APM_PSTAT_SUSPEND,	"suspend"	},
  { APM_PSTAT_OFF,	"off"		},
  { 0, 0 },
};

struct xlate pm_args[] = {
  { APM_DISABLE,	"disable"	},
  { APM_ENABLE,		"enable"	},
  { 0, 0},
};

void dflt_print(struct apmreq *);

struct cmds {
  char *name;
  struct xlate *args;
  struct apmreq req;
  void (*print)(struct apmreq *);
} cmds[] = {
  {
    "check",	0,
    {
      APM_INSTALLATION_CHECK,
      APM_DEV_BIOS,
    },
    dflt_print,
  }, {
    "connect",	0,
    {
      APM_CONNECT_P32,
      APM_DEV_BIOS,
    },
    dflt_print,
  }, {
    "disconnect",	0,
    {
      APM_DISCONNECT,
      APM_DEV_BIOS,
    },
    dflt_print,
  }, {
    "cpu_idle",		0,
    {
      APM_CPU_IDLE,
      APM_DEV_BIOS,
    },
    dflt_print,
  }, {
    "cpu_busy",		0,
    {
      APM_CPU_BUSY,
      APM_DEV_BIOS,
    },
    dflt_print,
  }, {
    "set_power_state",	pstat_args,
    {
      APM_SET_POWER_STATE,
      APM_DEV_ALL,
    },
    dflt_print,
  }, {
    "pm_control",	pm_args,
    {
      APM_PM_CONTROL,
      0xffff,
    },
    dflt_print,
  }, {
    "restore_default",	0,
    {
      APM_RESTORE_DEFAULT,
      0xffff,
    },
    dflt_print,
  }, {
    "power_status",	0,
    {
      APM_GET_POWER_STATUS,
      APM_DEV_ALL,
    },
    dflt_print,
  }, {
    "get_event",	0,
    {
      APM_GET_PM_EVENT,
      APM_DEV_ALL,
    },
    dflt_print,
  }, {
    "ready",		no_args,
    {
      APM_SET_POWER_STATE,
      APM_DEV_ALL,
      APM_PSTAT_READY,
    },
    dflt_print,
  }, {
    "standby",		no_args,
    {
      APM_SET_POWER_STATE,
      APM_DEV_ALL,
      APM_PSTAT_STANDBY,
    },
    dflt_print,
  }, {
    "suspend",		no_args,
    {
      APM_SET_POWER_STATE,
      APM_DEV_ALL,
      APM_PSTAT_SUSPEND,
    },
    dflt_print,
  }, {
    "off",		no_args,
    {
      APM_SET_POWER_STATE,
      APM_DEV_ALL,
      APM_PSTAT_OFF,
    },
    dflt_print,
  }, {
    0
  }
};


/*
 * sort of a reverse polish thing here, but quick and dirty
 */
main(int c, char **v) {
  int fd, status, i, j;
  char *dev = 0, *arg = 0, *fun = 0;
  register struct cmds *cp;
  struct apmreq *rq;


  if ((fd = open(_PATH_DEVAPM, O_RDWR)) < 0 &&
      (fd = open(_PATH_DEVAPM, O_RDONLY)) < 0) {
    perror(_PATH_DEVAPM);
    exit(1);
  }

  if (c == 1) {
    usage();
    exit (0);
  }

  for (i = 1; i < c; ++i) {
    if (strncmp(v[i], "-h", 2) == 0) {
      usage();
      continue;
    }

    if (strncmp(v[i], "dev=", 4) == 0) {
      dev = &v[i][4];
      if (!*dev)
	dev = 0;
      continue;
    }

    if (strncmp(v[i], "arg=", 4) == 0) {
      arg = &v[i][4];
      if (!*arg)
	arg = 0;
      continue;
    }

    if (strncmp(v[i], "fun=", 4) == 0) {
      static struct cmds custom = { "custom", 0, {0,0,}, dflt_print };

      /*
       * function override: pass custom function in
       */
      fun = &v[i][4];
      if (!*fun) {
	fun = 0;
	continue;
      }

      cp = &custom;
      rq = &cp->req;
      rq->func = lookup("fun=", fun, 0, "custom");
      if (*fun == '?')
	continue;

      goto doit;
    }

    /*
     * must be a command name: look for it in the command tab
     */
    for (cp = cmds; cp->name; ++cp) {
      if (strcmp(v[i], cp->name) == 0) {
	rq = &cp->req;
      doit:
	if (dev)
	  rq->dev = lookup("dev=", dev, devtab, cp->name);

	if (arg) {
	  if (*arg != '?' && cp->args == no_args) {
	    fprintf(stderr, "no args allowed for `%s'\n", cp->name);
	    exit(2);	/* not entirely friendly	*/
	  }
	  rq->arg = lookup("arg=", arg, cp->args, cp->name);
	}

	if ((dev && *dev == '?') || (arg && *arg == '?'))
	  break;

	if ((status = ioctl(fd, PIOCAPMREQ, rq)) != 0) {
		perror("PIOCAPMREQ");
		exit(1);
	}

	(*cp->print)(rq);
	break;
      }
    }

    if (cp->name == 0) {
      fprintf(stderr, "%s: unknown command\n", v[i]);
      exit(2);
    }
  }

  exit(0);
}

void
dflt_print(struct apmreq *req) {
  register struct xlate *ep;

  printf("func %x dev %x arg %x err %x aret %x bret %x cret %x\n"
	 , req->func, req->dev, req->arg
	 , req->err, req->aret, req->bret, req->cret);

  if (req->err) {
    for (ep = pm_errors; ep->name; ++ep) {
      if (ep->value == req->err) {
	printf("pm_err %s\n", ep->name);
	break;
      }
    }
  }
}

lookup(char *what, char *sym, struct xlate *tab, char *name) {
  int i, n;

  if (*sym == '?') {
    if (tab == no_args)
      printf("no arguments allowed for %s\n", name);
    else {
      printf("valid options for %s are:\n", name);
      if (tab == 0)
	printf("%20.20s\n", "<integer>");
      else while (tab->name) {
	printf("%20.20s	(0x%x)\n", tab->name, tab->value);
	++tab;
      }
    }
    return -1;
  }

  if (tab == 0) {
    /*
     * no symbol table: only <integer> allowed (0xhexval works as well)
     */
    n = sscanf(sym, "%i", &i);
    if (n != 1) {
      fprintf(stderr, "`%s' is not a valid integer value\n", sym);
      exit(3);
    }
    return i;
  }

  while (tab->name && strcmp(sym, tab->name))
    ++tab;

  if (!tab->name) {
    fprintf(stderr, "%s%s not valid\n", what, sym);
    exit(2);
  }

  return tab->value;
}

/*
 * usage -	print command usage
 */
usage() {
  register i;
  register struct xlate *xp;
  register struct cmds *cp;

  printf("usage: apm [dev=<dev>] [arg=<arg>] command\n");
  printf("  where `command' is one of:\n");

  for (cp = cmds; cp->name; ++cp)
    printf("	%s\n", cp->name);

  printf("  and `dev' is one of:\n\t<integer>\n");

  for (xp = devtab; xp->name; ++xp)
    printf("	%s\n", xp->name);

  printf("  and `arg' is one of:\n\t<integer>\n");
}
