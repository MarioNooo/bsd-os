/*	BSDI stats_inet6.h,v 2.5 2001/04/12 20:20:10 dab Exp	*/
/*----------------------------------------------------------------------
 inet.c:    netstat subroutines for AF_INET/AF_INET6.

 Originally from 4.4-Lite BSD.  Modifications to support IPv6 and IPsec
 copyright by Bao Phan, Randall Atkinson, & Dan McDonald, All Rights Reserved.
 All rights under this copyright are assigned to NRL.
----------------------------------------------------------------------*/
/*----------------------------------------------------------------------
#	@(#)COPYRIGHT	1.1 (NRL) 17 January 1995

COPYRIGHT NOTICE

All of the documentation and software included in this software
distribution from the US Naval Research Laboratory (NRL) are
copyrighted by their respective developers.

Portions of the software are derived from the Net/2 and 4.4 Berkeley
Software Distributions (BSD) of the University of California at
Berkeley and those portions are copyright by The Regents of the
University of California. All Rights Reserved.  The UC Berkeley
Copyright and License agreement is binding on those portions of the
software.  In all cases, the NRL developers have retained the original
UC Berkeley copyright and license notices in the respective files in
accordance with the UC Berkeley copyrights and license.

Portions of this software and documentation were developed at NRL by
various people.  Those developers have each copyrighted the portions
that they developed at NRL and have assigned All Rights for those
portions to NRL.  Outside the USA, NRL has copyright on some of the
software developed at NRL. The affected files all contain specific
copyright notices and those notices must be retained in any derived
work.

NRL LICENSE

NRL grants permission for redistribution and use in source and binary
forms, with or without modification, of the software and documentation
created at NRL provided that the following conditions are met:

1. All terms of the UC Berkeley copyright and license must be followed.
2. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
3. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
4. All advertising materials mentioning features or use of this software
   must display the following acknowledgements:

	This product includes software developed by the University of
	California, Berkeley and its contributors.

	This product includes software developed at the Information
	Technology Division, US Naval Research Laboratory.

5. Neither the name of the NRL nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation
are those of the authors and should not be interpreted as representing
official policies, either expressed or implied, of the US Naval
Research Laboratory (NRL).

----------------------------------------------------------------------*/


/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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

int ip6s_src_sameif, ip6s_src_otherif, ip6s_src_samescope,
    ip6s_src_otherscope, ip6s_src_deprecated, ip6s_m2m_sum;

static struct stats ip6_stat[] = {
	{ { offsetof(struct ip6stat, ip6s_total) },
	  S_FMT1, FMT(FMT_Q " total pkt%s rcvd") },
	{ { offsetof(struct ip6stat, ip6s_toosmall) },
	  S_FMT_5, FMT(FMT_Q " w/size smaller than minimum") },
	{ { offsetof(struct ip6stat, ip6s_tooshort) },
	  S_FMT_5, FMT(FMT_Q " w/data size < data length") },

	{ { offsetof(struct ip6stat, ip6s_badoptions) },
	  S_FMT_5, FMT(FMT_Q " w/bad options") },
	{ { offsetof(struct ip6stat, ip6s_badvers) },
	  S_FMT_5, FMT(FMT_Q " w/invalid IP version number") },
	{ { offsetof(struct ip6stat, ip6s_fragments) },
	  S_FMT1, FMT(FMT_Q " fragment%s rcvd") },
	{ { offsetof(struct ip6stat, ip6s_fragdropped) },
	  S_FMT1, FMT(FMT_Q " fragment%s dropped (dup or no bufs)") },
	{ { offsetof(struct ip6stat, ip6s_fragtimeout) },
	  S_FMT1, FMT(FMT_Q " fragment%s dropped after timeout") },
	{ { offsetof(struct ip6stat, ip6s_fragoverflow) },
	  S_FMT1, FMT(FMT_Q " fragment%s exceeded limit") },
	{ { offsetof(struct ip6stat, ip6s_reassembled) },
	  S_FMT1, FMT(FMT_Q " pkt%s reassembled ok") },
	{ { offsetof(struct ip6stat, ip6s_delivered) },
	  S_FMT1, FMT(FMT_Q " pkt%s for this host") },
	{ { offsetof(struct ip6stat, ip6s_forward) },
	  S_FMT1, FMT(FMT_Q " pkt%s forwarded") },
	{ { offsetof(struct ip6stat, ip6s_cantforward) },
	  S_FMT1, FMT(FMT_Q " pkt%s not forwardable") },
	{ { offsetof(struct ip6stat, ip6s_redirectsent) },
	  S_FMT1, FMT(FMT_Q " redirect%s sent") },
	{ { offsetof(struct ip6stat, ip6s_localout) },
	  S_FMT1, FMT(FMT_Q " pkt%s sent from this host") },
	{ { offsetof(struct ip6stat, ip6s_rawout) },
	  S_FMT1, FMT(FMT_Q " pkt%s sent w/fabricated ip header") },
	{ { offsetof(struct ip6stat, ip6s_odropped) },
	  S_FMT1, FMT(FMT_Q " output pkt%s dropped: no bufs, etc.") },
	{ { offsetof(struct ip6stat, ip6s_noroute) },
	  S_FMT1, FMT(FMT_Q " output pkt%s dropped: no route") },
	{ { offsetof(struct ip6stat, ip6s_fragmented) },
	  S_FMT1, FMT(FMT_Q " output dgram%s fragmented") },
	{ { offsetof(struct ip6stat, ip6s_ofragments) },
	  S_FMT1, FMT(FMT_Q " fragment%s created") },
	{ { offsetof(struct ip6stat, ip6s_cantfrag) },
	  S_FMT1, FMT(FMT_Q " dgram%s that can't be fragmented") },
	{ { offsetof(struct ip6stat, ip6s_badscope) },
	  S_FMT1, FMT(FMT_Q " packet%s that violated scope rules") },
	{ { offsetof(struct ip6stat, ip6s_notmember) },
	  S_FMT1, FMT(FMT_Q " multicast packet%s which we don't join") },


#define XH_FMT(n, str) \
	{ { offsetof(struct ip6stat, ip6s_nxthist[(n)]) }, \
	  S_FMT1|S_FMT_SKIPZERO, FMT_I(str ": " FMT_Q), (n) }
#define XH_NUL(n) \
	{ { offsetof(struct ip6stat, ip6s_nxthist[(n)]) }, \
	  S_FMT1|S_FMT_FLAG2|S_FMT_SKIPZERO, NULL, (n) }

	{ { 0 /*dummy*/},
	  S_FMT_5, FMT("Input histogram:") },
	XH_FMT(0, "hop by hop"),
	XH_FMT(1, "ICMP"),
	XH_FMT(2, "IGMP"),
	XH_NUL(3),
	XH_FMT(4, "IP"),
	XH_NUL(5),
	XH_FMT(6, "TCP"),
	XH_NUL(7), XH_NUL(8), XH_NUL(9), XH_NUL(10), XH_NUL(11), XH_NUL(12),
	XH_NUL(13), XH_NUL(14), XH_NUL(15), XH_NUL(16),
	XH_FMT(17, "UDP"),
	XH_NUL(18), XH_NUL(19), XH_NUL(20),
	XH_NUL(21),
	XH_FMT(22, "IDP"),
	XH_NUL(23), XH_NUL(24), XH_NUL(25), XH_NUL(26), XH_NUL(27), XH_NUL(28),
	XH_FMT(29, "TP"),
	XH_NUL(30), XH_NUL(31), XH_NUL(32), XH_NUL(33), XH_NUL(34), XH_NUL(35),
	XH_NUL(36), XH_NUL(37), XH_NUL(38), XH_NUL(39), XH_NUL(40),
	XH_FMT(41, "IP6"),
	XH_NUL(42),
	XH_FMT(43, "routing"),
	XH_FMT(44, "fragment"),
	XH_NUL(45), XH_NUL(46), XH_NUL(47), XH_NUL(48), XH_NUL(49),
	XH_FMT(50, "ESP"),
	XH_FMT(51, "AH"),
	XH_NUL(52), XH_NUL(53), XH_NUL(54), XH_NUL(55), XH_NUL(56), XH_NUL(57),
	XH_FMT(58, "ICMP6"),
	XH_FMT(59, "no next header"),
	XH_FMT(60, "destination option"),
	XH_NUL(61), XH_NUL(62), XH_NUL(63), XH_NUL(64), XH_NUL(65), XH_NUL(66),
	XH_NUL(67), XH_NUL(68), XH_NUL(69), XH_NUL(70), XH_NUL(71), XH_NUL(72),
	XH_NUL(73), XH_NUL(74), XH_NUL(75), XH_NUL(76), XH_NUL(77), XH_NUL(78),
	XH_NUL(79),
	XH_FMT(80, "ISOIP"),
	XH_NUL(81), XH_NUL(82), XH_NUL(83), XH_NUL(84), XH_NUL(85), XH_NUL(86),
	XH_NUL(87), XH_NUL(88),
	XH_FMT(89, "OSPF"),
	XH_NUL(90),
	XH_NUL(91), XH_NUL(92), XH_NUL(93), XH_NUL(94), XH_NUL(95), XH_NUL(96),
	XH_FMT(97, "Ethernet"),
	XH_NUL(98), XH_NUL(99), XH_NUL(100), XH_NUL(101), XH_NUL(102),
	XH_FMT(103, "PIM"),
	XH_NUL(104), XH_NUL(105),
	XH_NUL(106), XH_NUL(107), XH_NUL(108), XH_NUL(109), XH_NUL(110),
	XH_NUL(111), XH_NUL(112), XH_NUL(113), XH_NUL(114), XH_NUL(115),
	XH_NUL(116), XH_NUL(117), XH_NUL(118), XH_NUL(119), XH_NUL(120),
	XH_NUL(121), XH_NUL(122), XH_NUL(123), XH_NUL(124), XH_NUL(125),
	XH_NUL(126), XH_NUL(127), XH_NUL(128), XH_NUL(129), XH_NUL(130),
	XH_NUL(131), XH_NUL(132), XH_NUL(133), XH_NUL(134), XH_NUL(135),
	XH_NUL(136), XH_NUL(137), XH_NUL(138), XH_NUL(139), XH_NUL(140),
	XH_NUL(141), XH_NUL(142), XH_NUL(143), XH_NUL(144), XH_NUL(145),
	XH_NUL(146), XH_NUL(147), XH_NUL(148), XH_NUL(149), XH_NUL(150),
	XH_NUL(151), XH_NUL(152), XH_NUL(153), XH_NUL(154), XH_NUL(155),
	XH_NUL(156), XH_NUL(157), XH_NUL(158), XH_NUL(159), XH_NUL(160),
	XH_NUL(161), XH_NUL(162), XH_NUL(163), XH_NUL(164), XH_NUL(165),
	XH_NUL(166), XH_NUL(167), XH_NUL(168), XH_NUL(169), XH_NUL(170),
	XH_NUL(171), XH_NUL(172), XH_NUL(173), XH_NUL(174), XH_NUL(175),
	XH_NUL(176), XH_NUL(177), XH_NUL(178), XH_NUL(179), XH_NUL(180),
	XH_NUL(181), XH_NUL(182), XH_NUL(183), XH_NUL(184), XH_NUL(185),
	XH_NUL(186), XH_NUL(187), XH_NUL(188), XH_NUL(189), XH_NUL(190),
	XH_NUL(191), XH_NUL(192), XH_NUL(193), XH_NUL(194), XH_NUL(195),
	XH_NUL(196), XH_NUL(197), XH_NUL(198), XH_NUL(199), XH_NUL(200),
	XH_NUL(201), XH_NUL(202), XH_NUL(203), XH_NUL(204), XH_NUL(205),
	XH_NUL(206), XH_NUL(207), XH_NUL(208), XH_NUL(209), XH_NUL(210),
	XH_NUL(211), XH_NUL(212), XH_NUL(213), XH_NUL(214), XH_NUL(215),
	XH_NUL(216), XH_NUL(217), XH_NUL(218), XH_NUL(219), XH_NUL(220),
	XH_NUL(221), XH_NUL(222), XH_NUL(223), XH_NUL(224), XH_NUL(225),
	XH_NUL(226), XH_NUL(227), XH_NUL(228), XH_NUL(229), XH_NUL(230),
	XH_NUL(231), XH_NUL(232), XH_NUL(233), XH_NUL(234), XH_NUL(235),
	XH_NUL(236), XH_NUL(237), XH_NUL(238), XH_NUL(239), XH_NUL(240),
	XH_NUL(241), XH_NUL(242), XH_NUL(243), XH_NUL(244), XH_NUL(245),
	XH_NUL(246), XH_NUL(247), XH_NUL(248), XH_NUL(249), XH_NUL(250),
	XH_NUL(251), XH_NUL(252), XH_NUL(253), XH_NUL(254), XH_NUL(255),
#undef XH_NUL
#undef XH_FMT

	{ { 0 /*dummy*/},
	  S_FMT_5, FMT("Mbuf statistics:") },
	{ { offsetof(struct ip6stat, ip6s_m1) },
	  S_FMT_5, FMT_I(FMT_Q " one mbuf") },

#define MH_FMT(n) \
	{ { offsetof(struct ip6stat, ip6s_m2m[(n)]) }, \
	  S_FMT_5|S_FMT_FLAG1|S_FMT_SKIPZERO, NULL, (n) }

	{ { (u_long)&ip6s_m2m_sum },
	  S_FMT_5|S_FMT_DIRECT, FMT_I("two or more mbuf:") },
	MH_FMT(0), MH_FMT(1), MH_FMT(2), MH_FMT(3), MH_FMT(4),
	MH_FMT(5), MH_FMT(6), MH_FMT(7), MH_FMT(8), MH_FMT(9),
	MH_FMT(10), MH_FMT(11), MH_FMT(12), MH_FMT(13), MH_FMT(14),
	MH_FMT(15), MH_FMT(16), MH_FMT(17), MH_FMT(18), MH_FMT(19),
	MH_FMT(20), MH_FMT(21), MH_FMT(22), MH_FMT(23), MH_FMT(24),
	MH_FMT(25), MH_FMT(26), MH_FMT(27), MH_FMT(28), MH_FMT(29),
	MH_FMT(30), MH_FMT(31),

	{ { offsetof(struct ip6stat, ip6s_mext1) },
	  S_FMT_5, FMT_I(FMT_Q " one ext mbuf") },
	{ { offsetof(struct ip6stat, ip6s_mext2m) },
	  S_FMT_5, FMT_I(FMT_Q " two or more ext mbuf") },
	{ { offsetof(struct ip6stat, ip6s_exthdrtoolong) },
	  S_FMT1, FMT(FMT_Q " packet%s whose headers are not continuous") },
	{ { offsetof(struct ip6stat, ip6s_nogif) },
	  S_FMT1, FMT(FMT_Q " tunneling packet%s that can't find gif") },
	{ { offsetof(struct ip6stat, ip6s_toomanyhdr) },
	  S_FMT1, FMT(FMT_Q " packet%s discarded due to too many headers") },

#if 0
	{ { offsetof(struct ip6stat, ip6s_exthdrget) },
	  S_FMT1, FMT(FMT_Q " use%s of IP6_EXTHDR_GET") },
	{ { offsetof(struct ip6stat, ip6s_exthdrget0) },
	  S_FMT1, FMT(FMT_Q " use%s of IP6_EXTHDR_GET0") },
	{ { offsetof(struct ip6stat, ip6s_pulldown) },
	  S_FMT1, FMT(FMT_Q " call%s to m_pulldown") },
	{ { offsetof(struct ip6stat, ip6s_pulldown_alloc) },
	  S_FMT1, FMT(FMT_Q " mbuf allocation%s in m_pulldown") },
	{ { offsetof(struct ip6stat, ip6s_pulldown_copy) },
	  S_FMT1IES, FMT(FMT_Q " mbuf cop%s in m_pulldown") },
	{ { offsetof(struct ip6stat, ip6s_pullup) },
	  S_FMT1, FMT(FMT_Q " call%s to m_pullup") },
	{ { offsetof(struct ip6stat, ip6s_pullup_alloc) },
	  S_FMT1, FMT(FMT_Q " mbuf allocation%s in m_pullup") },
	{ { offsetof(struct ip6stat, ip6s_pullup_copy) },
	  S_FMT1IES, FMT(FMT_Q " mbuf cop%s in m_pullup") },
	{ { offsetof(struct ip6stat, ip6s_pullup_fail) },
	  S_FMT1, FMT(FMT_Q " failure%s in m_pullup") },
	{ { offsetof(struct ip6stat, ip6s_pullup2) },
	  S_FMT1, FMT(FMT_Q " call%s to m_pullup2") },
	{ { offsetof(struct ip6stat, ip6s_pullup2_alloc) },
	  S_FMT1, FMT(FMT_Q " mbuf allocation%s in m_pullup2") },
	{ { offsetof(struct ip6stat, ip6s_pullup2_copy) },
	  S_FMT1IES, FMT(FMT_Q " mbuf cop%s in m_pullup2") },
	{ { offsetof(struct ip6stat, ip6s_pullup2_fail) },
	  S_FMT1, FMT(FMT_Q " failure%s in m_pullup2") },
#endif

	{ { offsetof(struct ip6stat, ip6s_sources_none) },
          S_FMT1, FMT(FMT_Q " failure%s of source address selection") },


#define SC_FMT(stat, n, fmt) \
	{ { offsetof(struct ip6stat, ip6s_sources_##stat[n]) }, \
	  S_FMT1, FMT_I(FMT_Q " " fmt) }
#define SC_GFMT(stat, n) \
	{ { offsetof(struct ip6stat, ip6s_sources_##stat[n]) }, \
	  S_FMT1, FMT_I(FMT_Q " addresses scope=" #n) }

	{ { (u_long)&ip6s_src_sameif },
	  S_FMT_5|S_FMT_DIRECT, FMT("Source addresses on an outgoing I/F:") },
	SC_GFMT(sameif, 0),
	SC_FMT(sameif, 1, "node-local%s"),
	SC_FMT(sameif, 2, "link-local%s"),
	SC_GFMT(sameif, 3), SC_GFMT(sameif, 4),
	SC_FMT(sameif, 5, "site-local%s"),
	SC_GFMT(sameif, 6), SC_GFMT(sameif, 7), SC_GFMT(sameif, 8),
	SC_GFMT(sameif, 9), SC_GFMT(sameif, 10), SC_GFMT(sameif, 11),
	SC_GFMT(sameif, 12), SC_GFMT(sameif, 13),
	SC_FMT(sameif, 14, "global%s"),
	SC_GFMT(sameif, 15),

	{ { (u_long)&ip6s_src_otherif },
	  S_FMT_5|S_FMT_DIRECT, FMT("Source addresses on an non-outgoing I/F:") },
	SC_GFMT(otherif, 0),
	SC_FMT(otherif, 1, "node-local%s"),
	SC_FMT(otherif, 2, "link-local%s"),
	SC_GFMT(otherif, 3), SC_GFMT(otherif, 4),
	SC_FMT(otherif, 5, "site-local%s"),
	SC_GFMT(otherif, 6), SC_GFMT(otherif, 7), SC_GFMT(otherif, 8),
	SC_GFMT(otherif, 9), SC_GFMT(otherif, 10), SC_GFMT(otherif, 11),
	SC_GFMT(otherif, 12), SC_GFMT(otherif, 13),
	SC_FMT(otherif, 14, "global%s"),
	SC_GFMT(otherif, 15),

	{ { (u_long)&ip6s_src_samescope },
	  S_FMT_5|S_FMT_DIRECT, FMT("Source addresses of same scope:") },
	SC_GFMT(samescope, 0),
	SC_FMT(samescope, 1, "node-local%s"),
	SC_FMT(samescope, 2, "link-local%s"),
	SC_GFMT(samescope, 3), SC_GFMT(samescope, 4),
	SC_FMT(samescope, 5, "site-local%s"),
	SC_GFMT(samescope, 6), SC_GFMT(samescope, 7), SC_GFMT(samescope, 8),
	SC_GFMT(samescope, 9), SC_GFMT(samescope, 10), SC_GFMT(samescope, 11),
	SC_GFMT(samescope, 12), SC_GFMT(samescope, 13),
	SC_FMT(samescope, 14, "global%s"),
	SC_GFMT(samescope, 15),

	{ { (u_long)&ip6s_src_otherscope },
	  S_FMT_5|S_FMT_DIRECT, FMT("Source addresses of a different scope:") },
	SC_GFMT(otherscope, 0),
	SC_FMT(otherscope, 1, "node-local%s"),
	SC_FMT(otherscope, 2, "link-local%s"),
	SC_GFMT(otherscope, 3), SC_GFMT(otherscope, 4),
	SC_FMT(otherscope, 5, "site-local%s"),
	SC_GFMT(otherscope, 6), SC_GFMT(otherscope, 7),
	SC_GFMT(otherscope, 8), SC_GFMT(otherscope, 9),
	SC_GFMT(otherscope, 10), SC_GFMT(otherscope, 11),
	SC_GFMT(otherscope, 12), SC_GFMT(otherscope, 13),
	SC_FMT(otherscope, 14, "global%s"),
	SC_GFMT(otherscope, 15),

	{ { (u_long)&ip6s_src_deprecated },
	  S_FMT_5|S_FMT_DIRECT, FMT("Deprecated source addresses:") },
	SC_GFMT(deprecated, 0),
	SC_FMT(deprecated, 1, "node-local%s"),
	SC_FMT(deprecated, 2, "link-local%s"),
	SC_GFMT(deprecated, 3), SC_GFMT(deprecated, 4),
	SC_FMT(deprecated, 5, "site-local%s"),
	SC_GFMT(deprecated, 6), SC_GFMT(deprecated, 7),
	SC_GFMT(deprecated, 8), SC_GFMT(deprecated, 9),
	SC_GFMT(deprecated, 10), SC_GFMT(deprecated, 11),
	SC_GFMT(deprecated, 12), SC_GFMT(deprecated, 13),
	SC_FMT(deprecated, 14, "global%s"),
	SC_GFMT(deprecated, 15),

#undef SC_FMT
#undef SC_GFMT

	{ { 0 } }
};

ip6_stat_init(ip6s)
	struct ip6stat *ip6s;
{
	int i;
	struct stats *sp;
	char buf[LINE_MAX];

	ip6s_src_sameif = ip6s_src_otherif = ip6s_src_samescope = 0;
	ip6s_src_otherscope = ip6s_src_deprecated = ip6s_m2m_sum = 0;

	for (i = 0; i < 16; i++) {
		ip6s_src_sameif += ip6s->ip6s_sources_sameif[i];
		ip6s_src_otherif += ip6s->ip6s_sources_otherif[i];
		ip6s_src_samescope += ip6s->ip6s_sources_samescope[i];
		ip6s_src_otherscope += ip6s->ip6s_sources_otherscope[i];
		ip6s_src_deprecated += ip6s->ip6s_sources_deprecated[i];
	}
        for (i = 0; i < 32; i++)
		ip6s_m2m_sum += ip6s->ip6s_m2m[i];

	for (sp = ip6_stat; sp->s_flags != 0; sp++) {
		if ((sp->s_flags & S_FMT_FLAG1)) {
			char ibuf[IFNAMSIZ];

			if (if_indextoname(sp->s_aux, ibuf) != NULL) {
				snprintf(buf, sizeof(buf),
				    FMT_I("\t%s = %s"), ibuf, FMT_Q);
			} else {
				snprintf(buf, sizeof(buf),
				    FMT_I("\t#%u = %s"), sp->s_aux, FMT_Q);
			}
			sp->s_fmt = strdup(buf);
			continue;
		}
		if ((sp->s_flags & S_FMT_FLAG2)) {
			snprintf(buf, sizeof(buf),
			    FMT_I("#%u: %s"), sp->s_aux, FMT_Q);
			sp->s_fmt = strdup(buf);
		}
	}
}

static struct stats icmp6_stat[] = {
	{ { offsetof(struct icmp6stat, icp6s_error) },
	  S_FMT1, FMT(FMT_Q " call%s to icmp_error") },
	{ { offsetof(struct icmp6stat, icp6s_canterror) },
	  S_FMT1, FMT(FMT_Q " err%s not sent: old message was icmp err") },
	{ { offsetof(struct icmp6stat, icp6s_toofreq) },
	  S_FMT1, FMT(FMT_Q " err%s not sent: rate limited") },

	{ { (u_long)&icmp6s_sent },
	  S_FMT1|S_FMT_DIRECT, FMT(FMT_Q " pkt%s sent") },

#define HIST_FMT(n, fmt) \
	{ { offsetof(struct icmp6stat, icp6s_outhist[(n)]) }, \
	  S_FMT1|S_FMT_FLAG1|S_FMT_SKIPZERO, FMT_I(FMT_Q " " fmt "%s"), (n) }

#define HIST_FMT_IES(n, fmt) \
	{ { offsetof(struct icmp6stat, icp6s_outhist[(n)]) }, \
	  S_FMT1IES|S_FMT_FLAG1|S_FMT_SKIPZERO, FMT_I(FMT_Q " " fmt "%s"), (n) }

#define HIST_GFMT(n) \
	{ { offsetof(struct icmp6stat, icp6s_outhist[(n)]) }, \
	  S_FMT1|S_FMT_FLAG1|S_FMT_SKIPZERO, NULL, (n) }

#define HIST_GFMT10(n) HIST_GFMT(n), HIST_GFMT(n+1), HIST_GFMT(n+2), \
	HIST_GFMT(n+3), HIST_GFMT(n+4), HIST_GFMT(n+5), HIST_GFMT(n+6), \
	HIST_GFMT(n+7), HIST_GFMT(n+8), HIST_GFMT(n+9)

#define HIST_GFMT100(n) HIST_GFMT10(n), HIST_GFMT10(n+10), HIST_GFMT10(n+20), \
	HIST_GFMT10(n+30), HIST_GFMT10(n+40), HIST_GFMT10(n+50),\
	HIST_GFMT10(n+60), HIST_GFMT10(n+70), HIST_GFMT10(n+80), \
	HIST_GFMT10(n+90)

	HIST_FMT(ICMP6_DST_UNREACH, "destination unreachable"),
	HIST_FMT(ICMP6_PACKET_TOO_BIG, "pkt too big"),
	HIST_FMT(ICMP6_TIME_EXCEEDED, "time exceeded"),
	HIST_FMT(ICMP6_PARAM_PROB, "parameter problem"),
	HIST_FMT(ND_REDIRECT, "redirect"),

	HIST_GFMT(5), HIST_GFMT(6), HIST_GFMT(7),
	HIST_GFMT100(8), HIST_GFMT10(108), HIST_GFMT10(118),

	HIST_FMT(ICMP6_ECHO_REQUEST, "echo request"),
	HIST_FMT_IES(ICMP6_ECHO_REPLY, "echo repl"),
	HIST_FMT_IES(MLD6_LISTENER_QUERY, "group quer"),
	HIST_FMT(MLD6_LISTENER_REPORT, "group report"),
	HIST_FMT(MLD6_LISTENER_DONE, "group leave"),
	HIST_FMT(ND_ROUTER_SOLICIT, "router solicit"),
	HIST_FMT(ND_ROUTER_ADVERT, "router advert"),
	HIST_FMT(ND_NEIGHBOR_SOLICIT, "node solicit"),
	HIST_FMT(ND_NEIGHBOR_ADVERT, "node advert"),
	HIST_FMT(ND_REDIRECT, "redirect"),
	HIST_FMT(ICMP6_ROUTER_RENUMBERING, "router renumbering"),
	HIST_FMT(ICMP6_WRUREQUEST, "who are you (or FQDN) request"),
	HIST_FMT_IES(ICMP6_WRUREPLY, "who are you (or FQDN) repl"),
	HIST_FMT(MLD6_MTRACE_RESP, "mtrace response"),
	HIST_FMT(MLD6_MTRACE, "mtrace message"),


	{ { offsetof(struct icmp6stat, icp6s_badcode) },
	  S_FMT1, FMT(FMT_Q " msg%s w/bad code fields") },
	{ { offsetof(struct icmp6stat, icp6s_tooshort) },
	  S_FMT1, FMT(FMT_Q " msg%s < minimum length") },
	{ { offsetof(struct icmp6stat, icp6s_checksum) },
	  S_FMT1, FMT(FMT_Q " msg%s w/bad cksum") },
	{ { offsetof(struct icmp6stat, icp6s_badlen) },
	  S_FMT1, FMT(FMT_Q " msg%s w/bad length") },

	{ { (u_long)&icmp6s_received },
	  S_FMT1|S_FMT_DIRECT, FMT(FMT_Q " pkt%s rcvd") },

	HIST_GFMT(ICMP6_DST_UNREACH),
	HIST_GFMT(ICMP6_PACKET_TOO_BIG),
	HIST_GFMT(ICMP6_TIME_EXCEEDED),
	HIST_GFMT(ICMP6_PARAM_PROB),
	HIST_GFMT(ND_REDIRECT),

	HIST_GFMT(5), HIST_GFMT(6), HIST_GFMT(7),
	HIST_GFMT100(8), HIST_GFMT10(108), HIST_GFMT10(118),

	HIST_GFMT(ICMP6_ECHO_REQUEST),
	HIST_GFMT(ICMP6_ECHO_REPLY),
	HIST_GFMT(MLD6_LISTENER_QUERY),
	HIST_GFMT(MLD6_LISTENER_REPORT),
	HIST_GFMT(MLD6_LISTENER_DONE),
	HIST_GFMT(ND_ROUTER_SOLICIT),
	HIST_GFMT(ND_ROUTER_ADVERT),
	HIST_GFMT(ND_NEIGHBOR_SOLICIT),
	HIST_GFMT(ND_NEIGHBOR_ADVERT),
	HIST_GFMT(ND_REDIRECT),
	HIST_GFMT(ICMP6_ROUTER_RENUMBERING),
	HIST_GFMT(ICMP6_WRUREQUEST),
	HIST_GFMT(ICMP6_WRUREPLY),
	HIST_GFMT(MLD6_MTRACE_RESP),
	HIST_GFMT(MLD6_MTRACE),

#undef HIST_GFMT
#undef HIST_FMT
#undef HIST_FMT_IES

	{ { offsetof(struct icmp6stat, icp6s_reflect) },
	  S_FMT1, FMT(FMT_Q " message response%s generated") },

	{ { offsetof(struct icmp6stat, icp6s_nd_toomanyopt) },
	  S_FMT1, FMT(FMT_Q " message%s with too many ND options") },

	{ { offsetof(struct icmp6stat, icp6s_badns) },
	  S_FMT1, FMT(FMT_Q " bad neighbor solicitation message%s") },
	{ { offsetof(struct icmp6stat, icp6s_badna) },
	  S_FMT1, FMT(FMT_Q " bad neighbor advertisement message%s") },
	{ { offsetof(struct icmp6stat, icp6s_badrs) },
	  S_FMT1, FMT(FMT_Q " bad router solicitation message%s") },
	{ { offsetof(struct icmp6stat, icp6s_badra) },
	  S_FMT1, FMT(FMT_Q " bad router advertisement message%s") },
	{ { offsetof(struct icmp6stat, icp6s_badredirect) },
	  S_FMT1, FMT(FMT_Q " bad redirect message%s") },
	{ { offsetof(struct icmp6stat, icp6s_pmtuchg) },
	  S_FMT1, FMT(FMT_Q " path MTU change%s") },

	{ { 0 } }
};

static void
icmp6_stat_init(void) {
	struct stats *sp;
	char buf[LINE_MAX];
	const char *msgs[ICMP6_MAXTYPE + 1];
	u_int flags[ICMP6_MAXTYPE + 1];

	memset(msgs, 0, sizeof(msgs));
	memset(flags, 0, sizeof(flags));
	for (sp = icmp6_stat; sp->s_flags != 0; sp++) {
		if ((sp->s_flags & S_FMT_FLAG1) == 0)
			continue;
		if (sp->s_fmt != NULL) {
			msgs[sp->s_aux] = sp->s_fmt;
			flags[sp->s_aux] = sp->s_flags;
		} else if (msgs[sp->s_aux] == NULL) {
			(void)snprintf(buf, sizeof(buf), FMT_I("%s type #%u"),
			    FMT_Q, sp->s_aux);
			msgs[sp->s_aux] = sp->s_fmt = strdup(buf);
			flags[sp->s_aux] = sp->s_flags;
		} else {
			sp->s_fmt = msgs[sp->s_aux];
			sp->s_flags = flags[sp->s_aux];
		}
		sp->s_aux = 0;
	}
}

u_quad_t in_ahhist, in_esphist, in_comphist,
	out_ahhist, out_esphist, out_comphist;

static struct stats ipsec_stat[] = {
	{ { offsetof(struct ipsecstat, in_success) },
	  S_FMT1, FMT(FMT_Q  " inbound packet%s processed successfully") },
	{ { offsetof(struct ipsecstat, in_polvio) },
	  S_FMT1, FMT(FMT_Q  " inbound packet%s violated process security "
	  "policy") },
	{ { offsetof(struct ipsecstat, in_nosa) },
	  S_FMT1, FMT(FMT_Q  " inbound packet%s with no SA available") },
	{ { offsetof(struct ipsecstat, in_inval) },
	  S_FMT1, FMT(FMT_Q  " invalid inbound packet%s") },
	{ { offsetof(struct ipsecstat, in_nomem) },
	  S_FMT1, FMT(FMT_Q  " inbound packet%s failed due to insufficient "
	  "memory") },
	{ { offsetof(struct ipsecstat, in_badspi) },
	  S_FMT1, FMT(FMT_Q  " inbound packet%s failed getting SPI") },
	{ { offsetof(struct ipsecstat, in_ahreplay) },
	  S_FMT1, FMT(FMT_Q  " inbound packet%s failed on AH replay check") },
	{ { offsetof(struct ipsecstat, in_espreplay) },
	  S_FMT1, FMT(FMT_Q  " inbound packet%s failed on ESP replay check") },
	{ { offsetof(struct ipsecstat, in_ahauthsucc) },
	  S_FMT1, FMT(FMT_Q  " inbound packet%s considered authentic") },
	{ { offsetof(struct ipsecstat, in_ahauthfail) },
	  S_FMT1, FMT(FMT_Q  " inbound packet%s failed on authentication") },

#define NHIST(w, n, f) { { offsetof(struct ipsecstat, w[n]) }, \
	  S_FMT_5, FMT_I(f ": " FMT_Q) }
#define GHIST(w, n) { { offsetof(struct ipsecstat, w[n]) }, \
	  S_FMT_5, FMT_I("#" #n ": " FMT_Q) }
#define HIST_HDR(n, fmt) { { (u_long)&n }, \
	  S_FMT1|S_FMT_DIRECT, fmt ":\n" FMT(FMT_Q " total pkt%s rcvd") }

#define NHF(n, f) NHIST(in_ahhist, n, f)
#define NH(n) GHIST(in_ahhist, n)
#define NH5(n) NH(n), NH(n+1), NH(n+2), NH(n+3), NH(n+4)
#define NH10(n) NH5(n), NH5(n+5)
#define NH50(n) NH10(n), NH10(n+10), NH10(n+20), NH10(n+30), NH10(n+40)
#define NH100(n) NH50(n), NH50(n+50)

#undef	HIST_ARRAY
#define HIST_ARRAY in_ahhist
	HIST_HDR(in_ahhist, "AH input"),
        NHF(0, "none"),
        NHF(1, "hmac MD5"),
        NHF(2, "hmac SHA1"),
        NHF(3, "keyed MD5"),
        NHF(4, "keyed SHA1"),
        NHF(5, "null"),
	NH(6), NH(7), NH(8), NH(9), NH100(10), NH100(110),
	NH10(210), NH10(220), NH10(230), NH10(240), NH5(250), NH(255),

#undef	HIST_ARRAY
#define HIST_ARRAY in_esphist
	HIST_HDR(in_esphist, "ESP input"),
	NHF(0, "none"),
	NHF(1, "DES CBC"),
	NHF(2, "3DES CBC"),  
	NHF(3, "simple"),
	NHF(4, "blowfish CBC"),
	NHF(5, "CAST128 CBC"),
	NHF(6, "DES derived IV"),
	NH(7), NH(8), NH(9), NH100(10), NH100(110),
	NH10(210), NH10(220), NH10(230), NH10(240), NH5(250), NH(255),
 
#undef	HIST_ARRAY
#define HIST_ARRAY in_comphist
	HIST_HDR(in_comphist, "IPComp input"),
        NHF(0, "none"),
        NHF(1, "OUI"),
        NHF(2, "deflate"),
        NHF(3, "LZS"),
	NH(4), NH100(5), NH100(105), NH50(205), NH(255),

	{ { offsetof(struct ipsecstat, out_success) },
	  S_FMT1, FMT(FMT_Q  " outbound packet%s processed successfully") },
	{ { offsetof(struct ipsecstat, out_polvio) },
	  S_FMT1, FMT(FMT_Q  " outbound packet%s violated process security "
	  "policy") },
	{ { offsetof(struct ipsecstat, out_nosa) },
	  S_FMT1, FMT(FMT_Q  " outbound packet%s with no SA available") },
	{ { offsetof(struct ipsecstat, out_inval) },
	  S_FMT1, FMT(FMT_Q  " invalid outbound packet%s") },
	{ { offsetof(struct ipsecstat, out_nomem) },
	  S_FMT1, FMT(FMT_Q  " outbound packet%s failed due to insufficient "
	  "memory") },
	{ { offsetof(struct ipsecstat, out_noroute) },
	  S_FMT1, FMT(FMT_Q  " outbound packet%s with no route") },

#undef	HIST_ARRAY
#define HIST_ARRAY out_ahhist
	HIST_HDR(out_ahhist, "AH output"),
        NHF(0, "none"),
        NHF(1, "hmac MD5"),
        NHF(2, "hmac SHA1"),
        NHF(3, "keyed MD5"),
        NHF(4, "keyed SHA1"),
        NHF(5, "null"),
	NH(6), NH(7), NH(8), NH(9), NH100(10), NH100(110),
	NH10(210), NH10(220), NH10(230), NH10(240), NH5(250), NH(255),

#undef	HIST_ARRAY
#define HIST_ARRAY out_esphist
	HIST_HDR(out_esphist, "ESP output"),
	NHF(0, "none"),
	NHF(1, "DES CBC"),
	NHF(2, "3DES CBC"),  
	NHF(3, "simple"),
	NHF(4, "blowfish CBC"),
	NHF(5, "CAST128 CBC"),
	NHF(6, "DES derived IV"),
	NH(7), NH(8), NH(9), NH100(10), NH100(110),
	NH10(210), NH10(220), NH10(230), NH10(240), NH5(250), NH(255),
 
#undef	HIST_ARRAY
#define HIST_ARRAY out_comphist
	HIST_HDR(out_comphist, "IPComp output"),
        NHF(0, "none"),
        NHF(1, "OUI"),
        NHF(2, "deflate"),
        NHF(3, "LZS"),
	NH(4), NH100(5), NH100(105), NH50(205), NH(255),
};

ipsec_stat_init(ips)
	struct ipsecstat *ips;
{
	int i;
	struct stats *sp;
	char buf[LINE_MAX];

	in_ahhist = in_esphist = in_comphist = 0;
	out_ahhist = out_esphist = out_comphist = 0;

	for (i = 0; i < 256; i++) {
		in_ahhist += ips->in_ahhist[i];
		in_esphist += ips->in_esphist[i];
		in_comphist += ips->in_comphist[i];
		out_ahhist += ips->out_ahhist[i];
		out_esphist += ips->out_esphist[i];
		out_comphist += ips->out_comphist[i];
	}
}
