/*
 * WILDBOAR $Wildboar: GetClientInfo.c,v 1.3 1996/02/13 13:00:53 shigeya Exp $
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <i386/pcmcia/cs_conf.h>
#include <i386/pcmcia/cs_ioctl.h>

int
CS_GetClientInfo(fd, slot, cfg)
int fd, slot;
cs_cfg_t *cfg;
{
	csparam_t cs;

	cs.slot = slot;
	cs.cmd = CSC_GET_CLIENT_INFO;
	cs.data = cfg; 
	return(ioctl(fd, CSIOCREQ, &cs));
}
