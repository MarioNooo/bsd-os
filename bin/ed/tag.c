/*-
 * Copyright (c) 1999 Rodney Ruddock
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
#endif /* not lint */

#include <stdio.h>
#include <limits.h>
#include <strings.h>
#include <errno.h>

char *
strdiv(strg, match)
	char **strg;
	char match;
{
	char *ret=NULL;

	if (strg == NULL) {
		return(NULL);
	}

	ret = *strg;

	while (**strg != '\0') {
		if (**strg == match) {
			**strg = '\0';
			(*strg)++;
			return(ret);
		}
		(*strg)++;
	}

	return(ret);
}

FILE *
find_tag(tagname, filename)
	char *tagname;
	char *filename;
{
	FILE *l_fp;
	char l_line[PATH_MAX*2], *l_linep, *l_p;
	int l_len;

	l_fp = fopen("tags", "r");
	if (l_fp == NULL) {
		return(NULL);
	}
	for (;;) {
		l_linep = l_line;
		if (fgets(l_line, PATH_MAX*2, l_fp) == NULL) {
			fclose(l_fp);
			return(NULL);
		}
		l_p = strdiv(&l_linep, '\t');
		if (l_p == NULL) {
			continue;
		}
		if (strcmp(l_p, tagname) == 0) {
			l_p = strdiv(&l_linep, '\t');
			if (l_p == NULL) {
				continue;
			}
			strcpy(filename, l_p);
			l_len = strlen(l_linep);
			fseek(l_fp, -(l_len), SEEK_CUR);
			return(l_fp);
		}
	}

	fclose(l_fp);
	return(NULL);
}
