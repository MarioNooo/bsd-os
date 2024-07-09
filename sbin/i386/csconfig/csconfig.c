/*
 * WILDBOAR $Wildboar: csconfig.c,v 1.4 1996/02/13 13:01:04 shigeya Exp $
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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <i386/pcmcia/cs_conf.h>
#include <i386/pcmcia/cs_ioctl.h>

main(argc, argv)
int	argc;
char	**argv;
{
	int fd, flags;
	int slot, maxslot;

	if (argc != 2 && argc != 3)
		usage(argv[0]);

	if (strcmp(argv[1], "-a") == 0)
		slot = -1;
	else if (isdigit(*argv[1]))
		slot = atoi(argv[1]);
	else
		usage(argv[0]);

	if ((fd = CS_Open()) < 0) {
		perror("CS_Open");
		exit(1);
	}

	if ((maxslot = CS_GetMaxSlot(fd)) < 0) {
		perror("CS_GetMaxSlot");
		exit(1);
	}

	if (slot != -1 && (slot < 0 || slot >= maxslot)) {
		if (maxslot == 1)
			fprintf(stderr, "slot must be 0.\n");
		else
			fprintf(stderr,
				"slot must be in the range of 0 to %d.\n",
								maxslot - 1);
		exit(1);
	}

	if (argc == 2) {
		if (slot == -1) {
			for (slot = 0; slot < maxslot; slot++)
				getflag(fd, slot);
		} else
			getflag(fd, slot);
	} else {
		flags = 0;
		if (strcmp(argv[2], "up") == 0)
			flags = CSF_SLOT_UP;
		else if (strcmp(argv[2], "down") == 0)
			flags &= ~CSF_SLOT_UP;
		else
			usage(argv[0]);
		if (slot == -1) {
			for (slot = 0; slot < maxslot; slot++)
				setflag(fd, slot, flags);
		} else
			setflag(fd, slot, flags);
	}
	CS_Close(fd);
	exit(0);
}

usage(cmd)
char	*cmd;
{
	fprintf(stderr, "usage: %s slot [up|down]\n", cmd);
	exit(1);
}

setflag(fd, slot, flags)
int	fd, slot, flags;
{
	int status;

	if ((status = CS_SetSlotFlags(fd, slot, flags)) < 0)
			perror("CS_SetSlotFlags");
}

getflag(fd, slot)
int	fd, slot;
{
	int flags;

	if ((flags = CS_GetSlotFlags(fd, slot)) < 0) {
		perror("CS_GetSlotFlags");
		return;
	}
	printf("Slot %d: flags=0x%x<", slot, flags);
	printf((flags & CSF_SLOT_UP)? "UP" : "DOWN");
	if (flags & CSF_SLOT_EMPTY) {
		printf(",EMPTY>\n");
		return;
	}
	if (flags & CSF_SLOT_UP) {
		printf("%s>\n", (flags & CSF_SLOT_RUNNING)?
				",RUNNING" : ",UNKNOWN_CARD");
		getinfo(fd, slot, flags);
	} else
		printf(">\n");
}

char *cs_funcname[] = {
	"Multi Function",
	"Memory",
	"Serial",
	"Parallel",
	"PC card ATA",
	"Video",
	"LAN",
	"AIMS"
} ;

getinfo(fd, slot, flags)
int	fd, slot, flags;
{
	cs_cfg_t cfg;

	if (CS_GetClientInfo(fd, slot, &cfg) < 0) {
		perror("CS_GetClientInfo");
		return;
	}
	if (flags & CSF_SLOT_RUNNING)
		printf("\tAttached device: %s\n", cfg.cscfg_devname);
	print_vers1(cfg.cscfg_V1_str, cfg.cscfg_V1_len);
	printf("\tFunction ID: ");
	if (cfg.cscfg_FuncCode == -1)
		printf("None\n");
	else if (cfg.cscfg_FuncCode < 0 ||
		 cfg.cscfg_FuncCode >= (sizeof(cs_funcname) / sizeof(char *)))
		printf("%d (Unknown ID)\n", cfg.cscfg_FuncCode);
	else
		printf("%d (%s)\n", cfg.cscfg_FuncCode,
					cs_funcname[cfg.cscfg_FuncCode]);
	
	printf("\tAssigned IRQ: ");
	if (cfg.cscfg_AssignedIRQ != -1)
		printf("%d\n", cfg.cscfg_AssignedIRQ);
	else
		printf("None\n");
	printf("\tAssigned I/O port1: ");
	if (cfg.cscfg_BasePort1 != 0)
		printf("0x%x-0x%x\n", cfg.cscfg_BasePort1,
				cfg.cscfg_BasePort1 + cfg.cscfg_NumPorts1 - 1);
	else
		printf("None\n");
	if (cfg.cscfg_BasePort2 != 0)
		printf("\tAssigned I/O port2: 0x%x-0x%x\n",
				cfg.cscfg_BasePort2,
				cfg.cscfg_BasePort2 + cfg.cscfg_NumPorts2 - 1);
}

print_vers1(str, len)
u_char	*str;
int	len;
{

#define SKIP	{ int n = strlen((caddr_t)str) + 1; str += n; len -= n; }

	if (len == 0)		/* sanity */
		return;
	str += 2;		/* skip Version */
	len -= 2;
	printf("\tManufacturer Name: \"%s\"\n", str);
	SKIP;
	printf("\tProduct Name: \"%s\"\n", str);
	SKIP;
	if (len <= 0 || *str == 0 || *str == 0xff)
		return;
	printf("\tAdditional Info1: \"%s\"\n", str);
	SKIP;
	if (len <= 0 || *str == 0 || *str == 0xff)
		return;
	printf("\tAdditional Info2: \"%s\"\n", str);
}
