/*
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * Copyright (c) 1995 SDL Communications, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI n2setup.c,v 1.4 1997/07/24 13:50:32 cp Exp
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <unistd.h>

#include <i386/isa/if_n2reg.h>
#include <i386/isa/if_n2ioctl.h>
#include "n2setup.h"

extern void openfile __P((char *));

/*	Notes
 *
 *	The idle code register has been changed from 0x00 to 0xff
 *	to put 1's in unused fractional T1 channels. This was done
 *     	to improve clock sync on fractional T1 line installations.
 *					SDL	December 14, 1995
 *
 *	The line driver configuration register is addressed
 *	through the framer at address 0x7c. The B8ZS bit in this
 *	field should not be set. The B8ZS bit in the coding field
 *	causes this bit to be set as needed.
 *						July 19, 1995 cp
 *
 *	The first rx channel blocking register is at 0x32;
 *	The first tx channel blocking register is at 0x6c;
 *	It appears that these registers along with the line
 *	driver configuration register are the only active
 *	framer registers. 
 *						July 19, 1995 cp
 *
 *	The pulse density enforcement bit is not used to
 *	turn on pulse density enforcement. It is now used
 *	for 56k. This really means only use 7 our of 8 bits
 *	in the t1 character.
 *						July 19 1995 cp
 *
 *
 *	As code came from SDL the only options for the FDL
 *	register is default after reset or if ANSII not set then
 *	all zeros. Currently SDL does not believe we will ever
 *	be turning on any of these bits.
 *						July 19 1995 cp
 */



typedef struct csucmd {
	u_char	c_command[N2_CSU_COMMAND_SIZE];
	u_char	c_response[N2_CSU_RESPONSE_SIZE];
	u_char	c_status;
} csucmd_t;


n2_setupinfo_t si;

char ifname[IFNAMSIZ];
int fd;
n2_type_t  channeltype;

usage() {
	fprintf(stderr, "usage: n2setup");
	fprintf(stderr, 
		"[-u #] [input_file] \n");
	fprintf(stderr, "\t-u which ntwo unit\n");
	exit (1);
}

void
check4std()
{
	if ((channeltype == N2CSU) || (channeltype == N2DDS))
		yyerror("non-csu/non-dds command on csu/dds channel");
}

void
check4csu()
{
	if (channeltype == N2CSU)
		return;
	yyerror("csu command on non-csu channel");
}

void
check4dds()
{
	if (channeltype == N2DDS)
		return;
	yyerror("dds command on non-dds channel");
}


void
check4csu_or_dds()
{
	if (channeltype == N2CSU || channeltype == N2DDS)
		return;
	yyerror("csu or dds command on non-csu/non-dds channel");
}

static void
setb(int offset, u_char data)
{
	n2ioc_io_t io;

	io.io_offset = offset;
	io.io_data = data;
	strcpy(io.io_name, ifname);
	if (ioctl(fd, N2IOCSBYTE, (caddr_t)&io))
		err(1, "ioctl(2) N2IOCSBYTE said");
}

static u_char
getb(int offset)
{
	n2ioc_io_t io;

	io.io_offset = offset;
	strcpy(io.io_name, ifname);
	if (ioctl(fd, N2IOCGBYTE, (caddr_t)&io))
		err(1, "ioctl(2) N2IOCSBYTE said");
	return (io.io_data);
}

static
wait_for_ctw()
{
	int toolong;
	
	for (toolong = 0; toolong < 100; toolong++) {
		if (getb(N2_ICR) & N2_CTW_INT)
			return (0);
		usleep(50000);
	}
	return (1);
}

static int
wait_for_ctr()
{
	int toolong;	

	for (toolong = 0; toolong < 100; toolong++) {
		if (getb(N2_ICR) & N2_CTR_INT)
			return (0);
		usleep(50000);
	}	
	return (1);	

}

static void
csu_command(c)
	csucmd_t *c;
{
	int i;

	for (i = 0; i < N2_CSU_COMMAND_SIZE; i++) {
		if (wait_for_ctw()) {
			c->c_status = 1;
			return;
		}
		setb(N2_CDI, c->c_command[i]);
	}

	if (wait_for_ctr()) {
		c->c_status = 2;
		return;
	}
	c->c_response[0] =  getb(N2_CDI);
	if (c->c_response[0] == N2_CSU_RESPONSE_SIZE) {
		c->c_status = 5;
		return;
	}

	for (i = 1; i <= c->c_response[0]; i++) {
		if (wait_for_ctr()) {
		    c->c_status = 3;
		    return;
		}
		c->c_response[i] = getb(N2_CDI);

	}
	c->c_status = 0;
}


static void
setup_dds()
{
	int i;
	/*
	 * reset dsu
	 */
	setb(N2_CSR1, 0);
	usleep (1000000);
	i = getb(N2_CSR1);
	if (i != 0) 
		errx(1, "unable to zero register N2_CSR1 = %x", i);
	setb(N2_CSR1, N2_DDS_LIU_RUN);

	i = N2_DDS_LIU_RUN;
	if (si.si_intclk)
		i |= N2_DDS_INTCLK;
	if (si.si_tf72)
		i |= N2_DDS_9_6KB;
	setb(N2_CSR1, i);
	setb(N2_CSR2, getb(N2_CSR2) & ~(N2_DDS_LOOPBACK1 | N2_DDS_LOOPBACK2));
	setb(N2_CSR4, 0);
}


static void
setup_csu()
{
	csucmd_t cc;
	int i;
	n2ioc_int_t in;

	/*
	 * reset csu
	 */
	setb(N2_ICR, N2_SCA_INT);
	usleep (100000);
	setb(N2_ICR, N2_SCA_INT | N2_CSU_RUN); 
	usleep (2000000);

	/*
	 * set up the channel blocking registers
	 */
	cc.c_command[0] = N2_CSU_WRITE | N2_CSU_FRAMER_REGS;
	for (i = 0; i < 3; i++) 
	{
		cc.c_command[2] = si.si_cb[i];
		cc.c_command[1] = N2_RX_CHANNEL_BLK + i;
		csu_command(&cc);
		if (cc.c_status) 
		{
			errx(1, "set transmit cbr status = %x", cc.c_status);
		}
		cc.c_command[1] = N2_TX_CHANNEL_BLK + i;
		csu_command(&cc);
		if (cc.c_status)
		{
			errx(1, "set receive cbr status = %x", cc.c_status);
		}
		cc.c_command[2] = ~si.si_cb[i];	/* invert channel blocking */
		cc.c_command[1] = N2_TX_IDLE_REG + i;
		csu_command(&cc);
		if (cc.c_status) 
		{
			errx(1, "set transmit idle registers status = %x", cc.c_status);
		}
	}
	/* activate an all-ones idle code */
	cc.c_command[0] = N2_CSU_WRITE | N2_CSU_FRAMER_REGS;
	cc.c_command[1] = N2_IDLE_CODE_REG;
	cc.c_command[2] = N2_IDLE_CODE;	
	csu_command(&cc);
	if (cc.c_status) {
		errx(1, "set idle code status = %x", cc.c_status);
	}
	/*
	 * configure ICR
	 */
	i = N2_SCA_INT | N2_CSU_RUN;

	if (si.si_intclk)
		i |= N2_GENERATE_T1CLK;

	if (si.si_egl)
		i |= N2_EGL;
	setb(N2_ICR, i);

	/*
	 * set up line driver configuration
	 */
	cc.c_command[1] = N2_LDCR;
	switch (si.si_lbo) {
	case 0:
		cc.c_command[2] = N2_LDC_0DB;
		break;
	case 7:
		cc.c_command[2] = N2_LDC_7DB;
		break;
	case 15:
		cc.c_command[2] = N2_LDC_15DB;
		break;
	case 22:
		cc.c_command[2] = N2_LDC_22DB;
		break;
	}
	csu_command(&cc);
	if (cc.c_status) {
		errx(1, "set lbo status = %x", cc.c_status);
	}

	/*
	 * set line coding and frame mode
	 */
	cc.c_command[0] = N2_CSU_WRITE | N2_CSU_TYPE_CODING;
	cc.c_command[1] = 0;
	if (!si.si_ami)
		cc.c_command[1] |= N2_CSU_B8ZS;

	if (!si.si_esf)
		cc.c_command[1] |= N2_CSU_D4;

	if (si.si_rm56)
		cc.c_command[1] |= N2_CSU_56K;
	cc.c_command[2] = 0;
	csu_command(&cc);
	if (cc.c_status) {
		errx(1, "set coding status = %x", cc.c_status);
	}

	/*
	 * set up fdl
	 */
	cc.c_command[1] = 0;
	cc.c_command[0] = N2_CSU_WRITE | N2_CSU_FDL;
	cc.c_command[1] = 0;
	if (si.si_ansi_gen)
		cc.c_command[1] |= N2_FDL_ANSI_GENERATE;
	if (si.si_ansi_rx)
		cc.c_command[1] |= N2_FDL_ANSI_RX;
	if (si.si_ansi_tx)
		cc.c_command[1] |= N2_FDL_ANSI_TX;
	if (si.si_att)
		cc.c_command[1] |= N2_FDL_ATT;
	if (si.si_idle_code)
		cc.c_command[1] |= N2_FDL_IDLE_MARK;
	csu_command(&cc);
	if (cc.c_status)
		errx(1, "set ANSII status = %x", cc.c_status);

	strcpy (in.i_name, ifname);
	in.i_int = si.si_invertdata;
	if (ioctl(fd, N2IOCSPOLARITY, (caddr_t)&in))
		err(1, "ioctl(2) N2IOCSPOLARITY said");

}

static void
setup_noncsu()
{
	n2ioc_int_t in;

	strcpy (in.i_name, ifname);
	in.i_int = si.si_nodcd << N2NODCD_S;
	in.i_int |= si.si_sourceclock << N2SRCCLK_S;
	in.i_int |= si.si_rxclk2txclk << N2RX2TX_S;
	in.i_int |= (si.si_baud_rate_table_entry & N2RATE_M) << N2RATE_S;
	if (ioctl(fd, N2IOCSFLAGS, (caddr_t)&in))
		err(1, "ioctl(2) N2IOCSFLAGS said");
}

main(int argc, char **argv)
{
	int c;
	int unit;
	int i;
	n2ioc_gtype_t gt;

	

	strcpy(ifname, "ntwo0");


	while ((c = getopt(argc, argv, "u:")) != EOF)
		switch (c) {

		case 'u':
		    {
			char *endptr;

			unit  = strtol(optarg, &endptr, 0);
			if (endptr == optarg || unit < 0) {
				fprintf(stderr, "invalid unit specified\n");
				usage();
			}
			sprintf(ifname, "ntwo%d",unit);
			break;
		    }
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();

	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) 
		err(1, "socket(2) said");

	strcpy(gt.gt_name, ifname);
	if (ioctl(fd, N2IOCGTYPE, (caddr_t)&gt))
		err(1, "ioctl(2) N2IOCGTYPE said");
	channeltype = gt.gt_type;

	if (argc == 1)
		openfile(argv[0]);
	else
		openfile(NULL);
	yyparse();

	if (channeltype == N2CSU)
		setup_csu();

	if (channeltype == N2DDS)
		setup_dds();

	setup_noncsu();

	exit (0);
}
