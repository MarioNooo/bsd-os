/*
 * WILDBOAR $Wildboar: csctl.h,v 1.1 1996/03/17 08:35:14 shigeya Exp $
 */
/*
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

#include <i386/pcmcia/cs_spec.h>
#include <i386/pcmcia/cs_conf.h>

#ifndef _PATH_CARDCONF
#define	_PATH_CARDCONF	"/etc/pccard.conf"
#endif
#ifndef _PATH_CS
#define	_PATH_CS	"/dev/cs"
#endif
#ifndef _PATH_CSCTL_PID
#define	_PATH_CSCTL_PID	"/var/run/csctl.pid"
#endif

#define	MAXPARMLEVEL	16

/* csctl.c */
void reset_config(void);

/* kernio.c */
void kern_config(csspec_t **headp);
void kern_add(csspec_t *head);
void kern_reset(void);

/* daemon.c */
void start_daemon(void);

/* event.c */
int  parse_event(char **linep, csspec_t *cp, char *dname);
void free_event(void);
void show_event(char *dname, char *str, int len);
int  have_event(void);
void insert_event(int slot, cs_cfg_t *cfg);
void remove_event(int slot, cs_cfg_t *cfg);

/* parse.c */
void load_config(char *conffile, csspec_t **headp);
int  parse_quote(char **linep, char *buf, int len);
void show_quote(char *buf, int len);

/* param.c */
void define_parm(char **linep);
int  parse_parm(char **linep, char *dname, int idx);

extern char *cmd;
extern int qflag;
