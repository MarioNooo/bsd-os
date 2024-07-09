/*
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ttysetdev.c,v 2.1 1997/08/07 21:59:07 prb Exp
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <login_cap.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define	_PATH_FBTAB	"/etc/fbtab"

int
ttysetdev(char *name, uid_t uid, gid_t gid)
{
	char _name[256];
	char *buf, *dev, *line;
	FILE *fp;
	size_t len;
    	int mode;

	if (name == NULL)
		return (0);

	if (*name != '/') {
		snprintf(_name, sizeof(_name), _PATH_DEV "%s", name);
		name = _name;
	}

	if ((fp = fopen(_PATH_FBTAB, "r")) == NULL)
		return (0);

	if (secure_path(_PATH_FBTAB) < 0) {
		syslog(LOG_ERR, "%s: not secure!", _PATH_FBTAB);
		fclose(fp);
		return (-1);
	}

	while ((line = fgetln(fp, &len)) != NULL) {
		/*
		 * strip trailing newline, strip comments, skip blank lines
		 * and leading spaces.
		 */
		if (len > 0 && line[len-1] == '\n')
			line[len-1] = '\0';
		else
			line[len] = '\0';
		if ((buf = strchr(line, '#')) != NULL)
			*buf = '\0';
		buf = line;
		while (isspace(*buf))
			++buf;
		if (*buf == '#' || *buf == '\0')
			continue;

		/*
		 * Ignore lines not for our tty
		 */
		dev = name;
		while (*dev) {
			if (*dev != *buf)
				break;
			++buf;
			++dev;
		}
		if (*dev || !isspace(*buf))
			continue;

		/*
		 * we have a match, extract the tty modes
		 */
		while (isspace(*buf))
			++buf;

		mode = strtol(buf, &dev, 0);

		if (buf == dev || !isspace(*dev)) {
			syslog(LOG_ERR, "%s: invalid line: %s", _PATH_FBTAB, line);
			continue;
		}
		buf = ++dev;

		if (mode & ~(S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)) {
			syslog(LOG_ERR, "%s: invalid mode: %s", _PATH_FBTAB, line);
			continue;
		}

		/*
		 * now extact the file list, challing them as we go.
		 */
		while (isspace(*buf))
			++buf;

		while ((dev = buf) != NULL) {
			if ((buf = strchr(dev, ':')) != NULL)
				*buf++ = '\0';
			if (*dev == '\0')
				continue;
			if (*dev != '/') {
				syslog(LOG_ERR, "%s: bad device name", dev);
				continue;
			}
			if (chown(dev, uid, gid) < 0 || chmod(dev, mode) < 0) {
				syslog(LOG_ERR, "%s: chall(%d,%d,0%03o): %m",
				    dev, uid, gid, mode);
				continue;
			}
		}
	}
	fclose(fp);
	return (0);
}
