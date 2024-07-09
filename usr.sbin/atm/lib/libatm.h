/*
 *
 * ===================================
 * HARP  |  Host ATM Research Platform
 * ===================================
 *
 *
 * Copyright (c) 1996-1998, Network Computing Services, Inc.
 * All rights reserved.
 *
 *	@(#) libatm.h,v 1.5 1998/08/06 16:56:27 johnc Exp
 *
 */

/*
 * User Space Library Functions
 * ----------------------------
 *
 * Library functions
 *
 */

#ifndef _HARP_LIBHARP_H
#define _HARP_LIBHARP_H

/*
 * Start a HARP user-space timer
 *
 *	tp	pointer to timer control block
 *	time	number of seconds for timer to run
 *	fp	pointer to function to call at expiration
 */
#define HARP_TIMER(tp, time, fp)				\
{								\
	(tp)->ht_ticks = (time);				\
	(tp)->ht_mark = 0;					\
	(tp)->ht_func = (fp);					\
	LINK2HEAD((tp), Harp_timer, harp_timer_head, ht_next);	\
}

/*
 * Cancel a HARP user-space timer
 *
 *	tp	pointer to timer control block
 */
#define HARP_CANCEL(tp)						\
{								\
	UNLINK((tp), Harp_timer, harp_timer_head, ht_next);	\
}


/*
 * HARP user-space timer control block
 */
struct harp_timer {
	struct harp_timer	*ht_next;	/* Timer chain */
	int			ht_ticks;	/* Seconds till exp */
	int			ht_mark;	/* Processing flag */
	void			(*ht_func)();	/* Function to call */
};
typedef struct harp_timer	Harp_timer;


/*
 * Externally-visible variables and functions
 */

/* atm_addr.c */
extern int		get_hex_atm_addr __P((char *, u_char *, int));
extern char		*format_atm_addr __P((Atm_addr *));

/* cache_key.c */
extern void		scsp_cache_key __P((Atm_addr *,
				struct in_addr  *, int, char *));

/* ioctl_subr.c */
extern int		do_info_ioctl __P((struct atminfreq *, int));
extern int		get_vcc_info __P((char *,
				struct air_vcc_rsp **));
extern int		get_subnet_mask __P((char *,
				struct sockaddr_in *));
extern int		get_mtu __P((char *));
extern int		verify_nif_name __P((char *));

/* ip_addr.c */
extern struct sockaddr_in	*get_ip_addr __P((char *));
extern char		*format_ip_addr __P((struct in_addr *));

/* ip_checksum.c */
extern short		ip_checksum __P((char *, int));

/* timer.c */
extern Harp_timer	*harp_timer_head;
extern int		harp_timer_exec;
extern void		timer_proc __P(());
extern int		init_timer __P(());
extern int		block_timer __P(());
extern void		enable_timer __P((int));


#endif	/* _HARP_LIBHARP_H */
