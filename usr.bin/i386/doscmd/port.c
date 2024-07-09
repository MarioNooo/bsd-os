/*
 * Copyright (c) 1992, 1993, 1996
 *	Berkeley Software Design, Inc.  All rights reserved.
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
 *	This product includes software developed by Berkeley Software
 *	Design, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY Berkeley Software Design, Inc. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Berkeley Software Design, Inc. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI port.c,v 2.3 1998/01/29 16:51:57 donn Exp
 */

#include "doscmd.h"

#define	MINPORT		0x000
#define	MAXPORT_MASK	(MAXPORT - 1)

#include <sys/ioctl.h>
#include <i386/isa/pcconsioctl.h>
static int consfd = -1;

#define in(port) \
({ \
        register int _inb_result; \
\
        asm volatile ("xorl %%eax,%%eax; inb %%dx,%%al" : \
            "=a" (_inb_result) : "d" (port)); \
        _inb_result; \
})

#define out(port, data) \
        asm volatile ("outb %%al,%%dx" : : "a" (data), "d" (port))

FILE *iolog = 0;

static void
iomap(int port, int cnt)
{
    struct iorange r;

    if (consfd == -1 && (consfd = open("/dev/console", 0)) == -1) {
	perror("/dev/console");
	quit(1);
    }
    r.iobase = port;
    r.cnt = cnt + 3;
    if (ioctl(consfd, PCCONIOCMAPPORT, &r)) {
	perror("PCCONIOCMAPPORT");
	quit(1);
    }
}

static void
iounmap(int port, int cnt)
{
    struct iorange r;

    if (consfd == -1 && (consfd = open("/dev/console", 0)) == -1) {
	perror("/dev/console");
	quit(1);
    }
    r.iobase = port;
    r.cnt = cnt + 3;
    if (ioctl(consfd, PCCONIOCUNMAPPORT, &r)) {
	perror("PCCONIOCUNMAPPORT");
	quit(1);
    }
}

void
outb_traceport(int port, unsigned char byte)
{
/*
    if (!iolog && !(iolog = fopen("/tmp/iolog", "a")))
	iolog = stderr;

    fprintf(iolog, "0x%03X -> %02X\n", port, byte);
 */

    iomap(port, 1);
    out(port, byte);
    iounmap(port, 1);
}

unsigned char
inb_traceport(int port)
{
    unsigned char byte;

/*
    if (!iolog && !(iolog = fopen("/tmp/iolog", "a")))
	iolog = stderr;
 */

    iomap(port, 1);
    byte = in(port);
    iounmap(port, 1);

/*
    fprintf(iolog, "0x%03X <- %02X\n", port, byte);
    fflush(iolog);
 */
    return(byte);
}

/* 
 * Fake input/output ports
 */

static void
outb_nullport(int port, unsigned char byte)
{
/*
    debug(D_PORT, "outb_nullport called for port 0x%03X = 0x%02X.\n",
		   port, byte);
 */
}

static unsigned char
inb_nullport(int port)
{
/*
    debug(D_PORT, "inb_nullport called for port 0x%03X.\n", port);
 */
    return(0xff);
}

/*
 * configuration table for ports' emulators
 */

struct portsw {
	unsigned char	(*p_inb)(int port);
	void		(*p_outb)(int port, unsigned char byte);
} portsw[MAXPORT];

void
init_io_port_handlers(void)
{
    int i;

    for (i = 0; i < MAXPORT; i++) {
	if (portsw[i].p_inb == 0)
	    portsw[i].p_inb = inb_nullport;
	if (portsw[i].p_outb == 0)
	    portsw[i].p_outb = outb_nullport;
    }
}

void
define_input_port_handler(int port, unsigned char (*p_inb)(int port))
{
	if ((port >= MINPORT) && (port < MAXPORT)) {
		portsw[port].p_inb = p_inb;
	} else
		fprintf (stderr, "attempt to handle invalid port 0x%04x", port);
}

void
define_output_port_handler(int port, void (*p_outb)(int port, unsigned char byte))
{
	if ((port >= MINPORT) && (port < MAXPORT)) {
		portsw[port].p_outb = p_outb;
	} else
		fprintf (stderr, "attempt to handle invalid port 0x%04x", port);
}


void
inb(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	unsigned char (*in_handler)(int);

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port].p_inb;
	else
		in_handler = inb_nullport;
	b->tf_al = (*in_handler)(port);
	debug(D_PORT, "IN  on port %02x -> %02x\n", port, b->tf_al);
}

void
insb(struct trapframe *tf, int port)
{
	unsigned char (*in_handler)(int);
	unsigned char data;

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port].p_inb;
	else
		in_handler = inb_nullport;
	data = (*in_handler)(port);
	*(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di) = data;
	debug(D_PORT, "INS on port %02x -> %02x\n", port, data);

	if (tf->tf_eflags & PSL_D)
		tf->tf_di = (tf->tf_di - 1) & 0xffff;
	else
		tf->tf_di = (tf->tf_di + 1) & 0xffff;
}

void
inx(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	unsigned char (*in_handler)(int);

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port].p_inb;
	else
		in_handler = inb_nullport;
	b->tf_al = (*in_handler)(port);
	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port + 1].p_inb;
	else
		in_handler = inb_nullport;
	b->tf_ah = (*in_handler)(port + 1);
	debug(D_PORT, "IN  on port %02x -> %04x\n", port, tf->tf_ax);
}

void
insx(struct trapframe *tf, int port)
{
	unsigned char (*in_handler)(int);
	unsigned char data;

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port].p_inb;
	else
		in_handler = inb_nullport;
	data = (*in_handler)(port);
	*(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di) = data;
	debug(D_PORT, "INS on port %02x -> %02x\n", port, data);

	if ((port >= MINPORT) && (port < MAXPORT))
		in_handler = portsw[port + 1].p_inb;
	else
		in_handler = inb_nullport;
	data = (*in_handler)(port + 1);
	((u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di))[1] = data;
	debug(D_PORT, "INS on port %02x -> %02x\n", port, data);

	if (tf->tf_eflags & PSL_D)
		tf->tf_di = (tf->tf_di - 2) & 0xffff;
	else
		tf->tf_di = (tf->tf_di + 2) & 0xffff;
}

void
outb(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	void (*out_handler)(int, unsigned char);

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port].p_outb;
	else
		out_handler = outb_nullport;
	(*out_handler)(port, b->tf_al);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port, b->tf_al);
}

void
outx(struct trapframe *tf, int port)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	void (*out_handler)(int, unsigned char);

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port].p_outb;
	else
		out_handler = outb_nullport;
	(*out_handler)(port, b->tf_al);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port, b->tf_al);
	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port + 1].p_outb;
	else
		out_handler = outb_nullport;
	(*out_handler)(port + 1, b->tf_ah);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port + 1, b->tf_ah);
}

void
outsb(struct trapframe *tf, int port)
{
	void (*out_handler)(int, unsigned char);
	unsigned char value;

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port].p_outb;
	else
		out_handler = outb_nullport;
	value = *(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port, value);
	(*out_handler)(port, value);

	if (tf->tf_eflags & PSL_D)
		tf->tf_di = (tf->tf_di - 1) & 0xffff;
	else
		tf->tf_di = (tf->tf_di + 1) & 0xffff;
}

void
outsx(struct trapframe *tf, int port)
{
	void (*out_handler)(int, unsigned char);
	unsigned char value;

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port].p_outb;
	else
		out_handler = outb_nullport;
	value = *(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di);
	debug(D_PORT, "OUT on port %02x <- %02x\n", port, value);
	(*out_handler)(port, value);

	if ((port >= MINPORT) && (port < MAXPORT))
		out_handler = portsw[port + 1].p_outb;
	else
		out_handler = outb_nullport;
	value = ((u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di))[1];
	debug(D_PORT, "OUT on port %02x <- %02x\n", port+1, value);
	(*out_handler)(port + 1, value);

	if (tf->tf_eflags & PSL_D)
		tf->tf_di = (tf->tf_di - 2) & 0xffff;
	else
		tf->tf_di = (tf->tf_di + 2) & 0xffff;
}

unsigned char port_61 = 0x10;
int sound_on = 1;
int sound_freq = 1000;

void
outb_speaker(int port, unsigned char byte)
{
    if (raw_kbd) {
	if ((port_61 & 3) != 3) {
	    if ((byte & 3) == 3 && /* prtim[2].gate && */ sound_on)
		ioctl(kbd_fd, PCCONIOCSTARTBEEP, &sound_freq);
	} else if ((byte & 3) != 3)
	    ioctl(kbd_fd, PCCONIOCSTOPBEEP);
    }
    port_61 = byte; 
}

unsigned char
inb_speaker(int port)
{
    return(port_61);
}

void
speaker_init(void)
{
    define_input_port_handler(0x61, inb_speaker);
    define_output_port_handler(0x61, outb_speaker);
}
