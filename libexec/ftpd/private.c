/*-
 * Copyright (c) 1996,1998 Berkeley Software Design, Inc. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this notice is retained,
 * the conditions in the following notices are met, and terms applying
 * to contributors in the following notices also apply to Berkeley
 * Software Design, Inc.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *      Berkeley Software Design, Inc.
 * 4. Neither the name of the Berkeley Software Design, Inc. nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      BSDI private.c,v 1.2 1998/10/07 03:50:42 prb Exp
 *
 * This code is derived from code from wu-ftpd, though basically re-written
 * at BSDI.
 */
/*
 * Copyright (c) 1993, 1994  Washington University in Saint Louis
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * Washington University in Saint Louis and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASHINGTON UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASHINGTON
 * UNIVERSITY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <netinet/in.h>

#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>

#include "extern.h"
#include "ftpd.h"


struct acgrp {
	char	*gname;
	char	*gpass;
	char	*gr_name;
	gid_t	gr_gid;
	struct	acgrp *next;
};

static char *groupname;
static struct acgrp *acgrps;
static int group_attempts;

extern char remotehost[];
extern sigset_t allsigs;

void
init_groups(char *path)
{
	FILE *fp;
	struct stat finfo;
        struct acgrp *acg, *lacg;
	struct group *gr;
	char *line, *next, *p;

	if (path == NULL)
		return;

	if (stat(path, &finfo) != 0) {
		syslog(LOG_ERR, "cannot stat group access file %s: %m", path);
		return;
	}
	if (finfo.st_size == 0)
		return;

	if ((fp = fopen(path, "r")) == NULL) {
		if (errno != ENOENT) {
			syslog(LOG_ERR,
			    "cannot open group access file %s: %m", path);
		}
		return;
	}
	if (!(line = malloc(finfo.st_size + 2))) {
		syslog(LOG_ERR,
		    "could not allocate space group access file (%d bytes)",
		    finfo.st_size + 2);
		fclose(fp);
		return;
	}
	if (fread(line, finfo.st_size, 1, fp) != 1) {
		syslog(LOG_ERR, "error reading group access file %s: %m", path);
		fclose(fp);
		return;
	}
	fclose(fp);

	/*
	 * Make the last line be newline terminated, if not already.
	 */
	if (line[finfo.st_size - 1] != '\n')
		line[finfo.st_size++] = '\n';

	line[finfo.st_size] = '\0';

	acg = lacg = NULL;
	next = line;
	while ((line = next) && *line) {
		next = strchr(line, '\n');
		*next++ = '\0';

		if ((p = strchr(line, '#')) != NULL)
			*p = '\0';
		if (*line == '\0')
			continue;

		if (acg == NULL && (acg = malloc(sizeof(*acg))) == NULL) {
			syslog(LOG_ERR, "memory allocation failure");
			return;
		}
		acg->gname = line;
		acg->gpass = strchr(acg->gname, ':');
		if (acg->gpass == NULL) {
			syslog(LOG_ERR, "corrupted entry in group access file");
			continue;
		}
		*acg->gpass++ = '\0';
		acg->gr_name = strchr(acg->gpass, ':');
		if (acg->gr_name == NULL) {
			syslog(LOG_ERR, "corrupted entry in group access file");
			continue;
		}
		*acg->gr_name++ = '\0';
		if (strchr(acg->gr_name, ':') != NULL) {
			syslog(LOG_ERR, "corrupted entry in group access file");
			continue;
		}
		if ((gr = getgrnam(acg->gr_name)) == NULL) {
			syslog(LOG_ERR, "invalid group %s in group access file",
			    acg->gr_name);
			continue;
		}
		acg->gr_gid = gr->gr_gid;
		acg->next = NULL;
		if (lacg)
			lacg->next = acg;
		else
			acgrps = acg;
		lacg = acg;
		acg = NULL;
	}
}

void
group(char *group)
{
	if (FL(LOGGING))
		syslog(LOG_INFO,"SITE GROUP %s", group);
	if (groupname)
		free(groupname);
	groupname = sgetsave(group);
	reply(200, "Request for access to group %s accepted.", group);
}

void
gpass(char *gpass)
{
	char *xgpass, *salt;
	struct acgrp *grp;
	uid_t uid;

	if (FL(LOGGING))
		syslog(LOG_INFO,"SITE GPASS ???" );

	if (groupname == NULL) {
		reply(503, "Give group name with SITE GROUP first.");
		return;
	}

	for (grp = acgrps; grp; grp = grp->next)
		if (strcmp(groupname, grp->gname) == 0)
			break;
	free(groupname);
	groupname = NULL;

	if (grp == NULL)
		salt = "xx";
	else
		salt = grp->gpass;

	xgpass = crypt(gpass, salt);

	if (grp == NULL || gpass[0] == '\0' || strcmp(xgpass, grp->gpass)) {
		reply(530, "Group access request incorrect.");
		grp = NULL;
		if (group_attempts++ >= 5) {
			syslog(LOG_NOTICE,
			    "repeated group access failures from %s, group %s",
			    remotehost, groupname);
			exit(0);
		}
		sleep(group_attempts);  /* slow down password crackers */
		return;
	}

        sigprocmask (SIG_BLOCK, &allsigs, NULL);
	uid = geteuid();
	seteuid(0);
	setegid(grp->gr_gid);
	seteuid(uid);
        sigprocmask(SIG_UNBLOCK,&allsigs,NULL);


	reply(200, "Group access enabled.");
	group_attempts = 0;
}
