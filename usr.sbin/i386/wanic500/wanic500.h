/*
 * Copyright (c) 1999 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI wanic500.h,v 1.3 1999/07/29 16:18:30 cp Exp
 */

typedef struct wanic500 {
	int		w5_baudrate;
	int		w5_reset_delay;
	int		w5_tpackets;
	u_char		w5_csu_reset;
	u_char		w5_intclk;
	u_char		w5_ami;		/* not B8ZS */
	u_char		w5_rm56;	/* not 64k */
	u_char		w5_esf;		/* not SF (D4) */
	u_char		w5_egl;		/* equalizer gain limit on */
	u_char		w5_invertdata;	/* invert csu data */
	u_char		w5_ansi_gen;
	u_char		w5_ansi_rx;
	u_char		w5_ansi_tx;
	u_char		w5_att;
	u_char		w5_idle_code;
	u_char		w5_lbo;
	u_char		w5_pulse_density;
	u_char		w5_cb[3];
	u_char		w5_reg_v[sizeof (w5_info_t)];
	u_char		w5_reg_s[sizeof (w5_info_t)];
	u_char		w5_clockmode;
#define CM_DTE	0
#define CM_DCE	1
#define CM_SYM	2
	u_char		w5_rxquiet;
	u_char		w5_nodcd;		/* not sure about this */

} wanic500_t;

wanic500_t w5;

#define IS_V35	0x01
#define IS_CSU	0x02
extern void verify __P((int));
