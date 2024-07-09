/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI sysctlsubr.c,v 2.1 2002/02/20 19:39:04 torek Exp
 */

/*
 * Copyright (c) 1993
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

/*
 * need to verify this interface against FreeBSD's
 */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int	sysctlbyname(const char *, void *, size_t *, const void *, size_t);
int	sysctlnametomib(const char *, int *, size_t *);

#ifdef notyet

/* XXX - should really obtain these more dynamically */
static struct ctlname topname[] = CTL_NAMES;
static struct ctlname kernname[] = CTL_KERN_NAMES;
static struct ctlname vmname[] = CTL_KERN_NAMES;
static struct ctlname netname[] = CTL_NET_NAMES;
static struct ctlname hwname[] = CTL_HW_NAMES;
static struct ctlname username[] = CTL_USER_NAMES;
static struct ctlname *debugname;
static struct ctlname *vfsname;
#ifdef CTL_MACHDEP_NAMES
static struct ctlname machdepname[] = CTL_MACHDEP_NAMES;
#endif

struct list {
	struct	ctlname *list;
	int	size;
};
static struct list toplist = { topname, CTL_MAXID };
static struct list secondlevel[] = {
	{ 0, 0 },			/* CTL_UNSPEC */
	{ kernname, KERN_MAXID },	/* CTL_KERN */
	{ vmname, VM_MAXID },		/* CTL_VM */
	{ 0, 0 },			/* CTL_VFS */
	{ netname, NET_MAXID },		/* CTL_NET */
	{ 0, 0 },			/* CTL_DEBUG */
	{ hwname, HW_MAXID },
#ifdef CTL_MACHDEP_NAMES
	{ machdepname, CPU_MAXID },	/* CTL_MACHDEP */
#else
	{ 0, 0 },
#endif
	{ username, USER_MAXID },	/* CTL_USER_NAMES */
};

static char names[BUFSIZ];
static int lastused;

static struct ctlname vfs_generic_name[] = VFS_GENERIC_NAMES;
static struct list vfslist = { vfs_generic_name, VFS_MAXID };

/*
 * Initialize the set of debugging names
 */
static void
debuginit()
{
	size_t size;
	int mib[3], loc, i;
	int ctl_debug_maxid;

	if (secondlevel[CTL_DEBUG].list != 0)
		return;
	mib[0] = CTL_DEBUG;
	mib[1] = CTL_DEBUG_MAXID;
	mib[2] = CTL_DEBUG_NAME;
	size = sizeof(int);
	if (sysctl(mib, 3, &ctl_debug_maxid, &size, NULL, 0) == -1)
		return;
	debugname = malloc(ctl_debug_maxid * sizeof(struct ctlname));
	if (debugname == NULL)
		return;
	secondlevel[CTL_DEBUG].list = debugname;
	secondlevel[CTL_DEBUG].size = ctl_debug_maxid;
	for (loc = lastused, i = 0; i < ctl_debug_maxid; i++) {
		mib[1] = i;
		size = BUFSIZ - loc;
		if (sysctl(mib, 3, &names[loc], &size, NULL, 0) == -1)
			continue;
		debugname[i].ctl_name = &names[loc];
		debugname[i].ctl_type = CTLTYPE_INT;
		loc += size;
	}
	lastused = loc;
}

/*
 * Initialize the set of filesystem names
 * Allocate slot 0 for the generic identifier
 */
static void
vfsinit()
{
	struct vfsconf vfc;
	size_t buflen;
	int mib[4], maxtypenum, cnt, loc, size;

	if (secondlevel[CTL_VFS].list != 0)
		return;
	mib[0] = CTL_VFS;
	mib[1] = VFS_GENERIC;
	mib[2] = VFS_MAXTYPENUM;
	buflen = 4;
	if (sysctl(mib, 3, &maxtypenum, &buflen, NULL, (size_t)0) < 0)
		return;
	maxtypenum++;
	vfsname = malloc(maxtypenum * sizeof(*vfsname));
	if (vfsname == NULL)
		return;
	memset(vfsname, 0, maxtypenum * sizeof(*vfsname));
	loc = lastused;
	strcat(&names[loc], "generic");
	vfsname[0].ctl_name = &names[loc];
	vfsname[0].ctl_type = CTLTYPE_NODE;
	size = strlen(vfsname[0].ctl_name) + 1;
	loc += size;

	mib[2] = VFS_CONF;
	buflen = sizeof(vfc);
	for (cnt = 1; cnt < maxtypenum; cnt++) {
		mib[3] = cnt;
		if (sysctl(mib, 4, &vfc, &buflen, NULL, (size_t)0) < 0) {
			if (errno == EOPNOTSUPP)
				continue;
			/* ???
			warn("vfsinit");
			free(vfsname);
			return;
			??? */
			continue;
		}
		strcat(&names[loc], vfc.vfc_name);
		vfsname[cnt].ctl_name = &names[loc];
		vfsname[cnt].ctl_type = CTLTYPE_INT;
		size = strlen(vfc.vfc_name) + 1;
		loc += size;
	}
	lastused = loc;
	secondlevel[CTL_VFS].list = vfsname;
	secondlevel[CTL_VFS].size = maxtypenum;
}

static struct ctlname sockname[] = SOCTL_NAMES;
static struct list socklist = { sockname, SOCTL_MAXID };

static struct ctlname inetname[] = CTL_IPPROTO_NAMES;
static struct ctlname ipname[] = IPCTL_NAMES;
static struct ctlname icmpname[] = ICMPCTL_NAMES;
static struct ctlname igmpname[] = IGMPCTL_NAMES;
static struct ctlname tcpname[] = TCPCTL_NAMES;
static struct ctlname udpname[] = UDPCTL_NAMES;
static struct ctlname ipv6name[] = IPV6CTL_NAMES;
#if 0
static struct ctlname espname[] = ESPCTL_NAMES;
static struct ctlname ahname[] = AHCTL_NAMES;
#endif
static struct ctlname icmpv6name[] = ICMPV6CTL_NAMES;
static struct list inetlist = { inetname, IPPROTO_MAXID };
static struct list inetvars[] = {
	{ ipname, IPCTL_MAXID },	/* ip */
	{ icmpname, ICMPCTL_MAXID },	/* icmp */
	{ igmpname, IGMPCTL_MAXID },	/* igmp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ tcpname, TCPCTL_MAXID },	/* tcp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },
	{ udpname, UDPCTL_MAXID },	/* udp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ ipv6name, IPV6CTL_MAXID },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
#if 0
	{ espname, ESPCTL_MAXID },
	{ ahname, AHCTL_MAXID },
#else
	{ 0, 0 },	{ 0, 0 },
#endif
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },
	{ icmpv6name, ICMPV6CTL_MAXID },
};

static struct ctlname inet6name[] = CTL_IPV6PROTO_NAMES;
static struct list inet6list = { inet6name, IPV6PROTO_MAXID };
static struct list inet6vars[] = {
	{ ipv6name, IPCTL_MAXID },	/* ipv6 */
	{ icmpname, ICMPCTL_MAXID },	/* icmpv4 */
	{ igmpname, IGMPCTL_MAXID },	/* igmpv4 */
	{ 0, 0 },
	{ ipname, IPCTL_MAXID },	/* ipv4 */
	{ 0, 0 },
	{ tcpname, TCPCTL_MAXID },	/* tcp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },
	{ udpname, UDPCTL_MAXID },	/* udp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ ipv6name, IPV6CTL_MAXID },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
#if 0
	{ espname, ESPCTL_MAXID },
	{ ahname, AHCTL_MAXID },
#else
	{ 0, 0 },	{ 0, 0 },
#endif
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },
	{ icmpv6name, ICMPV6CTL_MAXID },
};

static struct ctlname routename[] = CTL_NET_ROUTE_NAMES;
static struct list routelist = { routename, NET_ROUTE_MAXID };

static struct ctlname arpname[] = ARPCTL_NAMES;
static struct list arplist = { arpname, ARPCTL_MAXID };

static struct list linklist = { NULL, 0 };

static struct ctlname linktypename[] = CTL_LINK_TYPE_NAMES;
static struct list linktypelist = { linktypename,
    sizeof(linktypename) / sizeof(linktypename[0]) };

#define CTL_LINKPROTO_MAXID	1
static struct ctlname linkprotoname[] = { { "numif", CTLTYPE_INT } };
static struct list linkvars[] = {
	{ 0, 0 },			/* 0x00 */
	{ 0, 0 },			/* 0x01 */
	{ 0, 0 },			/* 0x02 */
	{ 0, 0 },			/* 0x03 */
	{ 0, 0 },			/* 0x04 */
	{ 0, 0 },			/* 0x05 */
	{ 0, 0 },			/* 0x06 */
	{ 0, 0 },			/* 0x07 */
	{ 0, 0 },			/* 0x08 */
	{ 0, 0 },			/* 0x09 */
	{ 0, 0 },			/* 0x0a */
	{ 0, 0 },			/* 0x0b */
	{ 0, 0 },			/* 0x0c */
	{ 0, 0 },			/* 0x0d */
	{ 0, 0 },			/* 0x0e */
	{ 0, 0 },			/* 0x0f */
	{ 0, 0 },			/* 0x10 */
	{ 0, 0 },			/* 0x11 */
	{ 0, 0 },			/* 0x10 */
	{ 0, 0 },			/* 0x13 */
	{ 0, 0 },			/* 0x14 */
	{ 0, 0 },			/* 0x15 */
	{ 0, 0 },			/* 0x16 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 0x17 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 0x18 */
	{ 0, 0 },			/* 0x19 */
	{ 0, 0 },			/* 0x1a */
	{ 0, 0 },			/* 0x1b */
	{ 0, 0 },			/* 0x1c */
	{ 0, 0 },			/* 0x1d */
	{ 0, 0 },			/* 0x1e */
	{ 0, 0 },			/* 0x1f */
	{ 0, 0 },			/* 0x20 */
	{ 0, 0 },			/* 0x21 */
	{ 0, 0 },			/* 0x22 */
	{ 0, 0 },			/* 0x23 */
	{ 0, 0 },			/* 0x24 */
	{ 0, 0 },			/* 0x25 */
	{ 0, 0 },			/* 0x26 */
	{ 0, 0 },			/* 0x27 */
	{ 0, 0 },			/* 0x28 */
	{ 0, 0 },			/* 0x29 */
	{ 0, 0 },			/* 0x2a */
	{ 0, 0 },			/* 0x2b */
	{ 0, 0 },			/* 0x2c */
	{ 0, 0 },			/* 0x2d */
	{ 0, 0 },			/* 0x2e */
	{ 0, 0 },			/* 0x2f */
	{ 0, 0 },			/* 0x30 */
	{ 0, 0 },			/* 0x31 */
	{ 0, 0 },			/* 0x32 */
	{ 0, 0 },			/* 0x33 */
	{ 0, 0 },			/* 0x34 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 0x35 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 0x36 */

	{ 0, 0 },			/* 55 */
	{ 0, 0 },			/* 56 */
	{ 0, 0 },			/* 57 */
	{ 0, 0 },			/* 58 */
	{ 0, 0 },			/* 59 */

	{ 0, 0 },			/* 60 */
	{ 0, 0 },			/* 61 */
	{ 0, 0 },			/* 62 */
	{ 0, 0 },			/* 63 */
	{ 0, 0 },			/* 64 */
	{ 0, 0 },			/* 65 */
	{ 0, 0 },			/* 66 */
	{ 0, 0 },			/* 67 */
	{ 0, 0 },			/* 68 */
	{ 0, 0 },			/* 69 */
	{ 0, 0 },			/* 70 */
	{ 0, 0 },			/* 71 */
	{ 0, 0 },			/* 72 */
	{ 0, 0 },			/* 73 */
	{ 0, 0 },			/* 74 */
	{ 0, 0 },			/* 75 */
	{ 0, 0 },			/* 76 */
	{ 0, 0 },			/* 77 */
	{ 0, 0 },			/* 78 */
	{ 0, 0 },			/* 79 */
	{ 0, 0 },			/* 80 */
	{ 0, 0 },			/* 81 */
	{ 0, 0 },			/* 82 */
	{ 0, 0 },			/* 83 */
	{ 0, 0 },			/* 84 */
	{ 0, 0 },			/* 85 */
	{ 0, 0 },			/* 86 */
	{ 0, 0 },			/* 87 */
	{ 0, 0 },			/* 88 */
	{ 0, 0 },			/* 89 */
	{ 0, 0 },			/* 90 */
	{ 0, 0 },			/* 91 */
	{ 0, 0 },			/* 92 */
	{ 0, 0 },			/* 93 */
	{ 0, 0 },			/* 94 */
	{ 0, 0 },			/* 95 */
	{ 0, 0 },			/* 96 */
	{ 0, 0 },			/* 97 */
	{ 0, 0 },			/* 98 */
	{ 0, 0 },			/* 99 */
	{ 0, 0 },			/* 100 */
	{ 0, 0 },			/* 101 */
	{ 0, 0 },			/* 102 */
	{ 0, 0 },			/* 103 */
	{ 0, 0 },			/* 104 */
	{ 0, 0 },			/* 105 */
	{ 0, 0 },			/* 106 */
	{ 0, 0 },			/* 107 */
	{ 0, 0 },			/* 108 */
	{ 0, 0 },			/* 109 */
	{ 0, 0 },			/* 110 */
	{ 0, 0 },			/* 111 */
	{ 0, 0 },			/* 112 */
	{ 0, 0 },			/* 113 */
	{ 0, 0 },			/* 114 */
	{ 0, 0 },			/* 115 */
	{ 0, 0 },			/* 116 */
	{ 0, 0 },			/* 117 */
	{ 0, 0 },			/* 118 */
	{ 0, 0 },			/* 119 */
	{ 0, 0 },			/* 120 */
	{ 0, 0 },			/* 121 */
	{ 0, 0 },			/* 122 */
	{ 0, 0 },			/* 123 */
	{ 0, 0 },			/* 124 */
	{ 0, 0 },			/* 125 */
	{ 0, 0 },			/* 126 */
	{ 0, 0 },			/* 127 */
	{ 0, 0 },			/* 128 */
	{ 0, 0 },			/* 129 */
	{ 0, 0 },			/* 130 */
	{ 0, 0 },			/* 131 */
	{ 0, 0 },			/* 132 */
	{ 0, 0 },			/* 133 */
	{ 0, 0 },			/* 134 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 135 */
	{ 0, 0 },			/* 136 */
	{ 0, 0 },			/* 137 */
	{ 0, 0 },			/* 138 */
	{ 0, 0 },			/* 139 */
	{ 0, 0 },			/* 140 */
	{ 0, 0 },			/* 141 */
	{ 0, 0 },			/* 142 */
	{ 0, 0 },			/* 143 */
	{ 0, 0 },			/* 144 */
	{ 0, 0 },			/* 145 */
	{ 0, 0 },			/* 146 */
	{ 0, 0 },			/* 147 */
	{ 0, 0 },			/* 148 */
	{ 0, 0 },			/* 149 */
	{ 0, 0 },			/* 150 */
	{ 0, 0 },			/* 151 */
	{ 0, 0 },			/* 152 */
	{ 0, 0 },			/* 153 */
	{ 0, 0 },			/* 154 */
	{ 0, 0 },			/* 155 */
	{ 0, 0 },			/* 156 */
	{ 0, 0 },			/* 157 */
	{ 0, 0 },			/* 158 */
	{ 0, 0 },			/* 159 */
	{ 0, 0 },			/* 160 */
	{ 0, 0 },			/* 161 */
	{ 0, 0 },			/* 162 */
	{ 0, 0 },			/* 163 */
	{ 0, 0 },			/* 164 */
	{ 0, 0 },			/* 165 */
	{ 0, 0 },			/* 166 */
	{ 0, 0 },			/* 167 */
	{ 0, 0 },			/* 168 */
	{ 0, 0 },			/* 169 */
	{ 0, 0 },			/* 170 */
	{ 0, 0 },			/* 171 */
	{ 0, 0 },			/* 172 */
	{ 0, 0 },			/* 173 */
	{ 0, 0 },			/* 174 */
	{ 0, 0 },			/* 175 */
	{ 0, 0 },			/* 176 */
	{ 0, 0 },			/* 177 */
	{ 0, 0 },			/* 178 */
	{ 0, 0 },			/* 179 */
	{ 0, 0 },			/* 180 */
	{ 0, 0 },			/* 181 */
	{ 0, 0 },			/* 182 */
	{ 0, 0 },			/* 183 */
	{ 0, 0 },			/* 184 */
	{ 0, 0 },			/* 185 */
	{ 0, 0 },			/* 186 */
	{ 0, 0 },			/* 187 */
	{ 0, 0 },			/* 188 */
	{ 0, 0 },			/* 189 */
	{ 0, 0 },			/* 190 */
	{ 0, 0 },			/* 191 */
	{ 0, 0 },			/* 192 */
	{ 0, 0 },			/* 193 */
	{ 0, 0 },			/* 194 */
	{ 0, 0 },			/* 195 */
	{ 0, 0 },			/* 196 */
	{ 0, 0 },			/* 197 */
	{ 0, 0 },			/* 198 */
	{ 0, 0 },			/* 199 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 200 */
	{ 0, 0 },			/* 201 */
	{ 0, 0 },			/* 202 */
	{ 0, 0 },			/* 203 */
	{ 0, 0 },			/* 204 */
	{ 0, 0 },			/* 205 */
	{ 0, 0 },			/* 206 */
	{ 0, 0 },			/* 207 */
	{ 0, 0 },			/* 208 */
	{ 0, 0 },			/* 209 */
	{ 0, 0 },			/* 210 */
	{ 0, 0 },			/* 211 */
	{ 0, 0 },			/* 212 */
	{ 0, 0 },			/* 213 */
	{ 0, 0 },			/* 214 */
	{ 0, 0 },			/* 215 */
	{ 0, 0 },			/* 216 */
	{ 0, 0 },			/* 217 */
	{ 0, 0 },			/* 218 */
	{ 0, 0 },			/* 219 */
	{ 0, 0 },			/* 220 */
	{ 0, 0 },			/* 221 */
	{ 0, 0 },			/* 222 */
	{ 0, 0 },			/* 223 */
	{ 0, 0 },			/* 224 */
	{ 0, 0 },			/* 225 */
	{ 0, 0 },			/* 226 */
	{ 0, 0 },			/* 227 */
	{ 0, 0 },			/* 228 */
	{ 0, 0 },			/* 229 */
	{ 0, 0 },			/* 230 */
	{ 0, 0 },			/* 231 */
	{ 0, 0 },			/* 232 */
	{ 0, 0 },			/* 233 */
	{ 0, 0 },			/* 234 */
	{ 0, 0 },			/* 235 */
	{ 0, 0 },			/* 236 */
	{ 0, 0 },			/* 237 */
	{ 0, 0 },			/* 238 */
	{ 0, 0 },			/* 239 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 240 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 241 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 242 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 243 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 244 */
	{ 0, 0 },			/* 245 */
	{ 0, 0 },			/* 246 */
	{ 0, 0 },			/* 247 */
	{ 0, 0 },			/* 248 */
	{ 0, 0 },			/* 249 */
	{ 0, 0 },			/* 250 */
	{ 0, 0 },			/* 251 */
	{ 0, 0 },			/* 252 */
	{ 0, 0 },			/* 253 */
	{ 0, 0 },			/* 254 */
	{ 0, 0 },			/* 255 */
};

/* Interface specific */

static struct ctlname linkifname[] = CTL_LINK_NAMES;
static struct list linkiflist = { linkifname,
    sizeof(linkifname) / sizeof(linkifname[0]) };

/* Interface specific generic */

static struct ctlname linkgenname[] = LINK_GENERIC_NAMES;
static struct list linkgenlist = { linkgenname,
    sizeof(linkgenname) / sizeof(linkgenname[0]) };

/* Interface specific link type */

static struct ctlname ethertypename[] = ETHERCTL_NAMES;
static struct list ethertypelist = { ethertypename,
    sizeof(ethertypename) / sizeof(ethertypename[0]) };

static struct ctlname etherifname[IFT_ETHER + 1];
static struct list etheriflist = { etherifname,
    sizeof(etherifname) / sizeof(etherifname[0]) } ;

static struct ctlname fdditypename[] = FDDICTL_NAMES;
static struct list fdditypelist = { fdditypename,
    sizeof(fdditypename) / sizeof(fdditypename[0]) };

static struct ctlname fddiifname[IFT_FDDI + 1];
static struct list fddiiflist = { fddiifname,
    sizeof(fddiifname) / sizeof(fddiifname[0]) } ;

static struct ctlname tokentypename[] = TOKENCTL_NAMES;
static struct list tokentypelist = { tokentypename,
    sizeof(tokentypename) / sizeof(tokentypename[0]) };

static struct ctlname tokenifname[IFT_ISO88025 + 1];
static struct list tokeniflist = { tokenifname,
    sizeof(tokenifname) / sizeof(tokenifname[0]) } ;

static struct ctlname ieee80211typename[] = IEEE_802_11_NAMES;
static struct list ieee80211typelist = { ieee80211typename,
    sizeof(ieee80211typename) / sizeof(ieee80211typename[0]) };

static struct ctlname ieee80211ifname[IFT_IEEE80211 + 1];
static struct list ieee80211iflist = { ieee80211ifname,
    sizeof(ieee80211ifname) / sizeof(ieee80211ifname[0]) } ;

/* Interface specific HW type */

/* Interface specific Proto type */

static struct ctlname linknetname[] = CTL_LINK_NET_NAMES;
static struct list linknetlist = { linknetname,
    sizeof(linknetname) / sizeof(linknetname[0]) } ;

static struct ctlname inetifgenname[] = IPIFCTL_GEN_NAMES;
static struct list inetifgenlist = { inetifgenname,
    sizeof(inetifgenname) / sizeof(inetifgenname[0]) } ;

struct linkif {
	char	*if_name;		/* Pointer to interface name */
	struct list *if_linklist;	/* List for link type */
	struct list *if_namelist;	/* List of vars for link type */
};
static struct linkif *ifs;

static void
linkinit()
{
	struct linkif *lif;
	struct ifaddrs *ifa, *ifa0;
	struct sockaddr_dl *sdl;
	struct ctlname *lnp;
	int max_if;
	int i, *types;

	/* XXX - Should we rescan each time to find new interfaces? */
	if (linklist.list != NULL)
		return;

	if ((types = calloc(linktypelist.size + 1, sizeof(types[0]))) == NULL)
		return;

	etherifname[IFT_ETHER].ctl_name = "ether";
	etherifname[IFT_ETHER].ctl_type = CTLTYPE_NODE;
	fddiifname[IFT_FDDI].ctl_name = "fddi";
	fddiifname[IFT_FDDI].ctl_type = CTLTYPE_NODE;
	tokenifname[IFT_ISO88025].ctl_name = "iso88025";
	tokenifname[IFT_ISO88025].ctl_type = CTLTYPE_NODE;
	ieee80211ifname[IFT_IEEE80211].ctl_name = "ieee80211";
	ieee80211ifname[IFT_IEEE80211].ctl_type = CTLTYPE_NODE;
	    
	max_if = 0;
	if (getifaddrs(&ifa0) == 0)
		for (ifa = ifa0; ifa != NULL; ifa = ifa->ifa_next) {
			if ((sdl = (struct sockaddr_dl *)ifa->ifa_addr)
			    == NULL || ifa->ifa_addr->sa_family != AF_LINK)
				continue;
			if (sdl->sdl_index > max_if)
				max_if = sdl->sdl_index;
			if (sdl->sdl_type < linktypelist.size)
				types[sdl->sdl_type] = 1;
		}

	max_if++;
	if ((ifs = (struct linkif *)calloc(max_if, sizeof(*ifs))) == NULL) {
		free(types);
		return;
	}

	if ((lnp = (struct ctlname *)malloc(max_if * sizeof(*lnp))) == NULL) {
		free(types);
		return;
	}
	linklist.list = lnp;
	linklist.size = max_if;

	lnp->ctl_name = "generic";
	lnp->ctl_type = CTLTYPE_NODE;
	
	if (ifa0 == NULL) {
		free(types);
		return;
	}

	for (ifa = ifa0; ifa != NULL; ifa = ifa->ifa_next) {
		if ((sdl = (struct sockaddr_dl *)ifa->ifa_addr) == NULL ||
		    ifa->ifa_addr->sa_family != AF_LINK) 
			continue;

		/* Initialize per interface info */
		lif = &ifs[sdl->sdl_index];
		lif->if_name = ifa->ifa_name;
		switch (sdl->sdl_type) {
		case IFT_ETHER:
			lif->if_linklist = &etheriflist;
			lif->if_namelist = &ethertypelist;
			break;
		case IFT_FDDI:
			lif->if_linklist = &fddiiflist;
			lif->if_namelist = &fdditypelist;
			break;
		case IFT_ISO88025:
			lif->if_linklist = &tokeniflist;
			lif->if_namelist = &tokentypelist;
			break;
		case IFT_IEEE80211:
			lif->if_linklist = &ieee80211iflist;
			lif->if_namelist = &ieee80211typelist;
			break;
		}

		/* Initialize interface name lookup */
		lnp++;
		lnp->ctl_name = ifa->ifa_name;
		lnp->ctl_type = CTLTYPE_NODE;
	}

	for (i = 0; i < linktypelist.size; i++)
		if (!types[i])
			linktypelist.list[i].ctl_name = NULL;

	free(types);
}

/*
 * handle key requests
 */
static struct ctlname keynames[] = KEYCTL_NAMES;
static struct list keylist = { keynames, KEYCTL_MAXID };

#endif /* notyet */

int
sysctlbyname(const char *name, void *op, size_t *ol, const void *np, size_t nl)
{
	int mib[CTL_MAXNAME];
	size_t mibsize = CTL_MAXNAME;

	if (sysctlnametomib(name, mib, &mibsize))
		return (-1);
	return (sysctl(mib, mibsize, op, ol, np, nl));
}

int
sysctlnametomib(const char *name, int *mib, size_t *mibsize) {
#ifdef notyet
	size_t actualsize = 0;
	const char *cp = name;

	while (cp != NULL) {
		actualsize++;
		cp = strchr(cp, '.');
		if (cp != NULL)
			cp++;
	}
	if (*mibsize < actualsize) {
		errno = E2BIG;	/* ??? */
		return (-1);
	}
	... should read sysctls from file? ...
	*mibsize = actualsize;
	return (0);
#else /* notyet */
	errno = EOPNOTSUPP;
	return (-1);
#endif /* notyet */
}
