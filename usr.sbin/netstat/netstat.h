/*
 * Copyright (c) 1992, 1993
 *	Regents of the University of California.  All rights reserved.
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
 *
 *	@(#)netstat.h	8.2 (Berkeley) 1/4/94
 */

struct data_info {
	char	*di_name;	/* Data structure name */
	int	*di_mib;	/* Sysctl name */
	int	di_miblen;	/* Length of mib name */
	void	*di_ptr;	/* Pointer to data */
	size_t	di_size;	/* Structure size (for kvm_read) */
	u_long	di_off;		/* nlist offset in kernel */
	int	di_resolved;	/* non-zero if resolved */
};

extern struct	data_info tcb_info;	/* TCP pcb chain */
extern struct	data_info udb_info;	/* UDP pcb chain */

extern struct	data_info idp_info;

extern struct	data_info tp_info;
extern struct	data_info cltp_info;

extern int	Aflag;		/* show addresses of protocol control block */
extern int	aflag;		/* show all sockets (including servers) */
extern int	bflag;		/* show bytes instead of packets */
extern int	fflag;		/* address family */
extern int	gflag;		/* show group (multicast) routing or stats */
extern int	kflag;		/* use kmem instead of sysctl */
extern int	iflag;		/* show interfaces */
extern int	mflag;		/* show memory stats */
extern int	nflag;		/* show addresses numerically */
extern int	Oflag;		/* backward compatibility */
extern int	Pflag;		/* parsable output */
extern int	rflag;		/* show routing tables (or routing stats) */
extern int	sflag;		/* show protocol statistics */
extern int	vflag;		/* verbose */

extern int	interval;	/* repeat interval for i/f stats */

extern char	*interface;	/* desired i/f for stats, or NULL for all i/fs */

extern char	*filler;	/* Filler for empty fields */

extern int	resolve_quietly;	/* No error messages on sysctl() err */

extern char	*tcpstates[];

extern int	kread __P((u_long, void *, size_t));
extern int	skread __P((char *, struct data_info *));
extern void	resolve __P((char *, ...));

extern void	tcp_stats __P((char *));
extern void	udp_stats __P((char *));
extern void	ip_stats __P((char *));
extern void	icmp_stats __P((char *));
extern void	igmp_stats __P((char *));
extern void	inetprotopr __P((char *, struct data_info *));
extern void	ip6_stats __P((char *));
extern void	icmp6_stats __P((char *));
extern void	ipsec_stats __P((char *));

extern void	mbpr(void);

extern void	rt_stats __P((void));
extern char	*ns_phost __P((struct sockaddr *));

extern char	*af2name __P((int));
extern char	*routename __P((u_long));
extern char	*netname __P((u_long, u_long));
extern char	*ns_print __P((struct sockaddr *));
extern void	routepr __P(());
struct in_addr;
extern void	inetprint __P((int, void *, int, char *));
extern void	fillscopeid __P((struct sockaddr *));
extern void	p_sockaddr __P((struct sockaddr *, struct sockaddr *,
		    int, int));

extern void	nsprotopr __P((char *, struct data_info *));
extern void	spp_stats __P((char *));
extern void	idp_stats __P((char *));
extern void	nserr_stats __P((char *));

extern void	unixpr __P((void));

extern void	esis_stats __P((char *));
extern void	clnp_stats __P((char *));
extern void	cltp_stats __P((char *));
extern void	iso_protopr __P((char *, struct data_info *));
extern void	iso_protopr1 __P((u_long, int));
extern void	tp_protopr __P((u_long, char *));
extern void	tp_inproto __P((u_long));
extern void	tp_stats __P((char *));

extern void	mroutepr __P((void));
extern void	mrt_stats __P((void));

extern	void	if_print __P((void));
extern	void	if_print_interval __P((int));
extern	void	if_print_multi __P((void));
extern 	void	if_stats __P((void));

extern	void	mroute6pr __P((void));
extern	void	mrt6_stats __P((void));

/* Used in formats to unrestrict field sizes with -P */
#define	PARG(w, p)	Pflag ? 0 : (w), Pflag ? -1 : (p)

#define	PLURAL(n)	(n == 1 ? "" : "s")
#define	PLURALES(n)	(n == 1 ? "" : "es")
#define	PLURALIES(n)	(n == 1 ? "y" : "ies")

