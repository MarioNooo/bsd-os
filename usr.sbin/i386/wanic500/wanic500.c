/*
 * Copyright (c) 1999 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI wanic500.c,v 1.3 1999/07/29 16:18:30 cp Exp
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <unistd.h>

#include <i386/pci/if_w5ioctl.h>
#include <i386/pci/ic/hd64572.h>
#include <i386/pci/if_w5reg.h>
#include "wanic500.h"

extern void openfile __P((char *));

/*
 * calculate the byte offset of a struct element in w5_pim_csu
 */
#define PO(element)	((int)(&(((w5_pim_csu_t *)0)->element)))


typedef struct csucmd {
	u_char	c_cmd;
	u_char	c_cmd_data0;
	u_char	c_cmd_data1;
	u_char	c_cmd_data2;
	u_char	c_response[CSU_RESPONSE_SIZE];
	u_char	c_status;
} csucmd_t;

static int csu_waitdata __P((void));
static u_char get_pim __P((int offset));
static void csu_command __P((csucmd_t *));
static void set_pim __P((int offset, u_char data));
static void config_csu __P((w5_info_t *));
static void config_v35 __P((w5_info_t *));
static void usage __P((void));
static void w5_setbaud __P((int clk, int target, int *dcr, int *divr));


char ifname[IFNAMSIZ];
int fd;
static w5_info_t *info;

int
main(int argc, char **argv)
{
	int c;
	int unit;
	int i;
	int testmode = 0;

	w5_info_ioc_t info_ioc;



	info = &info_ioc.io_info;

	

	bzero(info, sizeof (*info));
	strcpy(ifname, "wf0");


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
			sprintf(ifname, "wf%d",unit);
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

	strcpy(info_ioc.io_name, ifname);
	if (ioctl(fd, W5IOCGIINFO, (caddr_t)&info_ioc))
		err(1, "W5IOCGIINFO");

	if (!info->i_inited) {
		/*
		 * these defaults came from SDL demo software
		 */
		info->i_trc0 = 50;
		info->i_trc0 = 61;
		info->i_tnr0 = 0x24;
		info->i_tnr1 = 0x30;
		info->i_tcr = 8;
		info->i_rrc = 11;
		info->i_rnr = 11;
		info->i_rcr = 51;
		info->i_tpackets = 5;
		info->i_inited = 1;
	}
	/*
	 * Default to all channels on t1.
	 *  active channels = all;
	 */
	w5.w5_cb[0] = 0xff;
	w5.w5_cb[1] = 0xff;
	w5.w5_cb[2] = 0xff;
	w5.w5_tpackets = info->i_tpackets;
	/*
	 * Default equalizer gain limit to on
	 *  equalizer gain limit = on;
	 */
	w5.w5_egl = 1;


	if (argc == 1)
		openfile(argv[0]);
	else
		openfile(NULL);
	yyparse();

	info->i_tpackets = w5.w5_tpackets;
	/*
	 * fetch updated register values
	 */
	for (i = 0; i < sizeof(w5_info_t); i++)
		if (w5.w5_reg_s[i])
			((char*)info)[i] = w5.w5_reg_v[i];


	switch (info->i_bid & PIM_MID_M)
	{
		case PIM_MID_V35:
			config_v35(info);
			break;
		case PIM_MID_T1:
			config_csu(info);
			break;
		default:
			errx(1, "unsupported physical interface module (pim)");
	}

	if (ioctl(fd, W5IOCSIINFO, (caddr_t)&info_ioc))
		err(1, "W5IOCSIINFO");

	exit (0);
}

void
verify(int types)
{
	switch (info->i_bid & PIM_MID_M) {
	case PIM_MID_V35:
		if ((types & IS_V35) == 0)
			yyerror("not applicable to V35 interface");
		break;
	case PIM_MID_T1:
		if ((types & IS_CSU) == 0)
			yyerror("not applicable to T1 CSU interface");
		break;
	}
}

void
usage()
{
	fprintf(stderr, "usage: wanic500");
	fprintf(stderr, 
		"[-u #] [input_file] \n");
	fprintf(stderr, "\t-u which ntwo unit\n");
	exit (1);
}

/*
 * Configure a v35 interface
 */
static void
config_v35(w5_info_t *info)
{
	int dcr;
	int divr;

	if (w5.w5_baudrate == 0)
		w5.w5_baudrate = 56000;

	w5_setbaud(32000000, w5.w5_baudrate, &dcr, &divr);
	switch (w5.w5_clockmode) {
	case CM_DTE:
		info->i_rxs = RXS_LINE;
		info->i_txs = TXS_LINE;
		info->i_pim_addr = 0x8;
		info->i_pim_data = 0x20;
		break;
	case CM_DCE:
		info->i_txs = divr | TXS_BRG | 0x80;
		info->i_rxs = divr | RXS_BRG;
		info->i_rtmc = dcr;
		info->i_ttmc = dcr;
		info->i_pim_addr = 0x8;
		info->i_pim_data = 0x11;
		break;
	case CM_SYM:
		info->i_txs = divr | TXS_BRG | 0x80;
		info->i_rxs = TXS_LINE;
		info->i_rtmc = dcr;
		info->i_ttmc = dcr;
		info->i_pim_addr = 0x8;
		info->i_pim_data = 0x21;
		break;
	default:
		errx(1, "internal error: unknown clockmode");
	}
}

/*
 * Configure A CSU.
 */
static void
config_csu(w5_info_t *info)
{
	csucmd_t cc;
	int i;
	/*
	 * reset csu
	 */
	if (w5.w5_csu_reset)
		set_pim(PO(pcsu_csr), CSR_RESET);
	set_pim(PO(pcsu_csr), 0);

	if (w5.w5_reset_delay)
		sleep(w5.w5_reset_delay);

	cc.c_cmd_data0 = 1;	/* channel 1; no channel 0 ?? */
	/*
	 * set up the channel blocking registers
	 */
	cc.c_cmd = CSU_WRITE | CSU_FRAMER_REGS;
	for (i = 0; i < 3; i++) {
		cc.c_cmd_data1 = CSU_RX_CHANNEL_BLK + i;
		cc.c_cmd_data2 = ~w5.w5_cb[i];
		csu_command(&cc);
		if (cc.c_status) 
			errx(1, "set transmit cbr status = %x", cc.c_status);
		cc.c_cmd_data1 = CSU_TX_CHANNEL_BLK + i;
		csu_command(&cc);
		if (cc.c_status)
			errx(1, "set receive cbr status = %x", cc.c_status);
		cc.c_cmd_data1 = CSU_TX_IDLE_REG + i;
		cc.c_cmd_data2 = ~w5.w5_cb[i];	/* invert channel blocking */
		csu_command(&cc);
		if (cc.c_status) 
			errx(1, "set transmit idle registers status = %x",
			    cc.c_status);
	}
	/* activate an all-ones idle code */
	cc.c_cmd_data1 = CSU_IDLE_CODE_REG;
	cc.c_cmd_data2 = CSU_IDLE_CODE;	
	csu_command(&cc);
	if (cc.c_status) {
		errx(1, "set idle code status = %x", cc.c_status);
	}

	i =  CSR_TX_CLK_INVRT | CSR_RX_CLK_INVRT; /* from sdl driver */
	if (w5.w5_intclk)
		i |= CSR_TX_XTAL;
	if (w5.w5_invertdata)
		i |= CSR_DATA_INVRT;
	if (w5.w5_rm56)
		i |= CSR_56K;
	info->i_pim_data = i;
	info->i_pim_addr = PO(pcsu_csr);
	info->i_rxs = RXS_LINE;
	info->i_txs = TXS_LINE;

	/*
	 * set line coding and frame mode
	 */
	cc.c_cmd = CSU_WRITE | CSU_TYPE_CODING;
	cc.c_cmd_data1 = 0;
	if (!w5.w5_ami)
		cc.c_cmd_data1 |= CSU_B8ZS;

	if (!w5.w5_esf)
		cc.c_cmd_data1 |= CSU_D4;

	if (w5.w5_pulse_density)
		cc.c_cmd_data1 |= CSU_PULSE_DENSITY;
	cc.c_cmd_data2 = 0;
	csu_command(&cc);
	if (cc.c_status) {
		errx(1, "set coding status = %x", cc.c_status);
	}

	/*
	 * set up line driver configuration
	 */
	cc.c_cmd = CSU_WRITE | CSU_LBO;
	switch (w5.w5_lbo) {
	case 0:
		cc.c_cmd_data1 = CSU_LDC_0DB;
		break;
	case 7:
		cc.c_cmd_data1 = CSU_LDC_7DB;
		break;
	case 15:
		cc.c_cmd_data1 = CSU_LDC_15DB;
		break;
	case 22:
		cc.c_cmd_data1 = CSU_LDC_22DB;
		break;
	}
	cc.c_cmd_data1 |= CSU_JITTER_SHALLOW;
	if (w5.w5_egl)
		cc.c_cmd_data1 |= CSU_EGL;
	csu_command(&cc);
	if (cc.c_status) {
		errx(1, "set lbo status = %x", cc.c_status);
	}

	/*
	 * set up fdl
	 */
	cc.c_cmd_data1 = 0;
	cc.c_cmd = CSU_WRITE | CSU_FDL;
	cc.c_cmd_data1 = 0;
	if (w5.w5_ansi_gen)
		cc.c_cmd_data1 |= CSU_FDL_ANSI_GENERATE;
	if (w5.w5_ansi_rx)
		cc.c_cmd_data1 |= CSU_FDL_ANSI_RX;
	if (w5.w5_ansi_tx)
		cc.c_cmd_data1 |= CSU_FDL_ANSI_TX;
	if (w5.w5_att)
		cc.c_cmd_data1 |= CSU_FDL_ATT;
	if (w5.w5_idle_code)
		cc.c_cmd_data1 |= CSU_FDL_IDLE_MARK;
	csu_command(&cc);
	if (cc.c_status)
		errx(1, "set facilities data link = %x", cc.c_status);

}

/*
 * divr is power of 2 devisor and goes in the clock source register
 * dcr is the down counting divisor and goes in tmc
 */
static void
w5_setbaud(int clk, int target, int *dcr, int *divr)
{
	int div;
	int dc;

	for (div = 1; div < 1024; div *=2) {
		dc = clk / (div * target);
		/*
		 * The div is after the down counter.  This
		 * means that a div of 1 with a dcr of other than
		 * 1 will produce an asymetrical clock. Take care
		 * of this case first. 
		 */
		if (div == 1 && dc > 1)
			continue;
		if (dc <= 256)
			break;
	}
	if (div == 1024) {
		/* can't go this slow */
		*divr = ffs(512) - 1;
		*dcr = 255;
		return;
	}
	*divr = ffs(div) - 1;
	*dcr = dc;
	if (dc == 256)
		return;
	/*
	 * check if round up is better
	 */
	if ((clk - dc * div * target) > (target * (dc + 1) * div - clk))
		(*dcr)++;
}

/*
 * Send a command to the processor on the CSU.
 * Fetch any results.
 */
static void
csu_command(csucmd_t *c)
{
	int i;

	set_pim(PO(pcsu_hdata0), c->c_cmd_data0);
	set_pim(PO(pcsu_hdata1), c->c_cmd_data1);
	set_pim(PO(pcsu_hdata2), c->c_cmd_data2);
	set_pim(PO(pcsu_cmd), c->c_cmd);

	if (csu_waitdata()) {
		c->c_status = 2;
		return;
	}
	c->c_response[0] =  get_pim(PO(pcsu_cdata));
	if (c->c_response[0] == CSU_RESPONSE_SIZE) {
		c->c_status = 5;
		return;
	}

	for (i = 1; i <= c->c_response[0]; i++) {
		if (csu_waitdata()) {
		    c->c_status = 3;
		    return;
		}
		c->c_response[i] = get_pim(PO(pcsu_cdata));
	}
	c->c_status = 0;
}

/*
 * Wait for a data byte to be available from the csu
 */
static int
csu_waitdata()
{
	int toolong;	

	for (toolong = 0; toolong < 100; toolong++) {
		if (get_pim(PO(pcsu_ctl)) & 0x10)
			return (0);
		usleep(50000);
	}	
	return (1);	
}

/*
 * Write a byte into the physical interface module
 */
static void
set_pim(int offset, u_char data)
{
	w5_pim_cmd_t io;

	io.io_offset = offset;
	io.io_data = data;
	strcpy(io.io_name, ifname);
	if (ioctl(fd, W5IOCSPIMB, (caddr_t)&io))
		err(1, "W5IOCSPIMB");
}

/*
 * Read a byte from the physical interface module
 */
static u_char
get_pim(int offset)
{
	w5_pim_cmd_t io;

	io.io_offset = offset;
	strcpy(io.io_name, ifname);
	if (ioctl(fd, W5IOCGPIMB, (caddr_t)&io))
		err(1, "W5IOCGPIMB");
	return (io.io_data);
}
