/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI n2setup.h,v 1.1 1995/08/17 15:57:59 cp Exp
 */

typedef struct n2_setupinfo {
	/*
	 * fields used with csu port
	 */
	u_char		si_intclk;
	u_char		si_ami;		/* not B8ZS */
	u_char		si_rm56;	/* not 64k */
	u_char		si_esf;		/* not SF (D4) */
	u_char		si_egl;		/* equalizer gain limit on */
	u_char		si_invertdata;	/* invert csu data */
	u_char		si_ansi_gen;
	u_char		si_ansi_rx;
	u_char		si_ansi_tx;
	u_char		si_att;
	u_char		si_idle_code;
	u_char		si_lbo;
	u_char		si_cb[3];

	/*
	 * fields used with dds port
	 */
	u_char		si_tf72;	/* transmission frequence not 56Kb */

	/*
	 * fields used with V.35 port
	 */
	u_char		si_rxclk2txclk;
	u_char		si_sourceclock;
	u_char		si_baud_rate_table_entry;

	/*
	 * following applies to all
	 */
	u_char		si_rxquiet;
	u_char		si_nodcd;		/* not sure about this */

} n2_setupinfo_t;
