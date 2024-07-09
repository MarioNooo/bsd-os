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
 *	BSDI doscmd.h,v 2.5 1999/09/15 16:27:22 donn Exp
 */

#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <machine/psl.h>
#include <machine/npx.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include "dos.h"
#include "cwd.h"
#include "config.h"

#define	MAX_AVAIL_SEG 0xa000
char *dosmem;
extern char cmdname[];
int dosmem_size;

int init_cs;
int init_ip;
int init_ss;
int init_sp;
int init_ds;
int init_es;

int pspseg;                                     /* segment # of PSP */
int curpsp;

struct timeval boot_time;
 
/*
 * From machdep.c
 */
struct sigframe {
  int	sf_signum;
  int	sf_code;
  struct sigcontext *sf_scp;
  sig_t	sf_handler;
  int	sf_eax;	
  int	sf_edx;	
  int	sf_ecx;	
  struct save87 sf_fpu;
  struct sigcontext sf_sc;
};

/*
 * From frame.h (slightly changed)
 * SHOULD USE trapframe_vm86 from <machine/frame.h>
 */
struct trapframe {
	int	tf_es386;
	int	tf_ds386;
	ushort	tf_di;
	ushort	:16;
	ushort	tf_si;
	ushort	:16;
	ushort	tf_bp;
	ushort	:16;
	int	tf_isp386;
	ushort	tf_bx;
	ushort	:16;
	ushort	tf_dx;
	ushort	:16;
	ushort	tf_cx;
	ushort	:16;
	ushort	tf_ax;
	ushort	:16;
	int	tf_trapno;
	/* below portion defined in 386 hardware */
	int	tf_err;
	ushort	tf_ip;
	ushort	:16;
	ushort	tf_cs;
	ushort	:16;
	int	tf_eflags;
	/* below only when transitting rings (e.g. user to kernel) */
	ushort	tf_sp;
	ushort	:16;
	ushort	tf_ss;
	ushort	:16;
	/* below only when transitting from vm86 */
	ushort	tf_es;
	ushort	:16;
	ushort	tf_ds;
	ushort	:16;
	ushort	tf_fs;
	ushort	:16;
	ushort	tf_gs;
	ushort	:16;
};

struct byteregs {
	u_char tf_bl;
	u_char tf_bh;
	u_short :16;
	u_char tf_dl;
	u_char tf_dh;
	u_short :16;
	u_char tf_cl;
	u_char tf_ch;
	u_short :16;
	u_char tf_al;
	u_char tf_ah;
	u_short :16;
};

struct vconnect_area {
        int     int_state;
        int     magic;                  /* 0x4242 -> PRB format */
        u_long  passthru[256>>5];       /* bitmap of INTs to handle */
        u_long  magiciret[2];           /* Bounds of "magic" IRET */
};

extern struct sigframe saved_sigframe;
extern struct trapframe saved_trapframe;

extern struct vconnect_area vconnect_area;
#define	IntState	vconnect_area.int_state

#define PRB_V86_FORMAT  0x4242

int vflag;
int tmode;

inline static char *
MAKE_ADDR(int sel, int off)
{

	return (char *)((sel << 4) + off);
}

inline static void
PUSH(u_short x, struct trapframe *frame)
{
	frame->tf_sp -= 2;
	*(u_short *)MAKE_ADDR(frame->tf_ss, frame->tf_sp) = x;
}

inline static u_short
POP(struct trapframe *frame)
{
	u_short x = *(u_short *)MAKE_ADDR(frame->tf_ss, frame->tf_sp);

	frame->tf_sp += 2;
	return (x);
}

inline static u_short
PEEK(struct trapframe *frame, int off)
{
	u_short x = *(u_short *)MAKE_ADDR(frame->tf_ss, frame->tf_sp + off * 2);
	return (x);
}

char *disk_transfer_addr;
int	disk_transfer_seg;
int	disk_transfer_offset;

u_char	*VREG;

#define	BIOSDATA	((u_char *)0x400)

unsigned long rom_config;

void dump_regs ();

int intnum;

void fake_int(struct trapframe *tf, int);

void set_irq_request(int);	/* XXX not implemented */

void mem_init(void);
int mem_alloc(int, int, int *);
int mem_adjust(int, int, int *);
void mem_change_owner(int, int);
void mem_free_owner(int);

__dead void fatal(char *fmt, ...) __attribute__((__volatile));
void debug(int flags, char *fmt, ...);

typedef void (*int_func_t)();
char *translate_filename(char *);

extern int_func_t int_func_table[256];
extern int	  int_table[512];

int debug_flags;
int exec_level;

/* Lower 8 bits are int number */
#define D_ALWAYS 	0x0000100
#define D_TRAPS 	0x0000200
#define D_FILE_OPS	0x0000400
#define D_MEMORY	0x0000800
#define D_HALF		0x0001000 /* for "half-implemented" system calls */
#define	D_FLOAT		0x0002000
#define	D_DISK		0x0004000
#define	D_TRAPS2	0x0008000
#define	D_PORT		0x0010000
#define	D_EXEC		0x0020000
#define	D_ITRAPS	0x0040000
#define	D_REDIR		0x0080000
#define	D_PRINTER	0x0100000
#define	D_TRAPS3	0x0200000
#define	D_DEBUGIN	0x0400000
#define	D_TEMP		0x80000000

#define	TTYF_ECHO	0x00000001
#define	TTYF_ECHONL	0x00000003
#define	TTYF_CTRL	0x00000004
#define	TTYF_BLOCK	0x00000008
#define	TTYF_POLL	0x00000010
#define	TTYF_REDIRECT	0x00010000	/* Cannot have 0xffff bits set */

#define	TTYF_ALL	(TTYF_ECHO | TTYF_CTRL | TTYF_REDIRECT)
#define	TTYF_BLOCKALL	(TTYF_ECHO | TTYF_CTRL | TTYF_REDIRECT | TTYF_BLOCK)

/*
 * Must match i386-pinsn.c
 */
#define es_reg		100
#define cs_reg		101
#define ss_reg		102
#define ds_reg		103
#define fs_reg		104
#define gs_reg		105

#define	bx_si_reg	0
#define	bx_di_reg	1
#define	bp_si_reg	2
#define	bp_di_reg	3
#define	si_reg		4
#define	di_reg		5
#define	bp_reg		6
#define	bx_reg		7

extern int redirect0;
extern int redirect1; 
extern int redirect2;
extern int xmode;
extern int dosmode;
extern int raw_kbd;
extern int kbd_fd;
extern int booting;
extern int use_fork;
extern unsigned long *ivec;
extern int jmp_okay;
extern int dead;
extern FILE *debugf;

extern int nfloppies;
extern int ndisks;
extern int nserial;
extern int nparallel;
extern int nmice;

extern struct sigframe saved_sigframe;
extern struct trapframe saved_trapframe;
extern int saved_valid;

int ParseBuffer(char *, char **, int);

void init_devinit_handlers(void);
void init_optional_devices(void);

void bios_init(void);
void cmos_init(void);

void define_input_port_handler(int, unsigned char (*)(int));
void define_output_port_handler(int, void (*)(int, unsigned char));
void init_io_port_handlers(void);
void speaker_init(void);

void switch_vm86(struct sigframe *, struct trapframe *);
void _switch_vm86(struct sigframe, struct trapframe);
int init_hdisk(int, int, int, int, char *, char *);
int init_floppy(int, int, char *);
int disk_fd(int);
void make_readonly(int);
void put_dosenv(char *);

int map_type(int, int *, int *, int *);

int search_floppy(int);

void printer_direct(int);
void printer_spool(int, char *);
void printer_timeout(int, char *);

int KbdEmpty(void);
u_short KbdPeek(void);
u_short KbdRead(void);
void KbdWrite(u_short);
void console_init(void);
void timer_init(void);
int tty_eread(struct trapframe *, int, int);
int tty_estate(void);
void tty_write(int, int);
void tty_rwrite(int, int);
void tty_move(int, int);
void tty_report(int *, int *);
void tty_flush(void);
void tty_index(void);
void tty_pause(void);
int tty_peek(struct trapframe *, int);
int tty_read(struct trapframe *, int);
int tty_state(void);
void tty_scroll(int, int, int, int, int, int);
void tty_rscroll(int, int, int, int, int, int);
int tty_char(int, int);
void video_bios_init(void);
void video_blink(int);
void video_init(void);
void video_setborder(int);

void mouse_init(void);

int canonicalize(char *);

void lpt_poll(void);
void reset_poll(void);
void sleep_poll(void);
void wakeup_poll(void);

void unknown_int2(int, int, struct trapframe *);
void unknown_int3(int, int, int, struct trapframe *);
void unknown_int4(int, int, int, int, struct trapframe *);

void trap(struct sigframe *, struct trapframe *);
void sigtrap(struct sigframe *, struct trapframe *);
void sigtrace(struct sigframe *, struct trapframe *);
void sigill(struct sigframe *, struct trapframe *);
void floating(struct sigframe *, struct trapframe *);
void random_sig_handler(struct sigframe *, struct trapframe *);
void breakpoint(struct sigframe *, struct trapframe *);
void tracetrap(struct trapframe *);
void resettrace(void);

void int09(struct trapframe *);
void int10(struct trapframe *);
void int11(struct trapframe *);
void int12(struct trapframe *);
void int13(struct trapframe *);
void int14(struct trapframe *);
void int15(struct trapframe *);
void int16(struct trapframe *);
void int17(struct trapframe *);
void int1a(struct trapframe *);
void int20(struct trapframe *);
void int21(struct trapframe *);
void int2f(struct trapframe *);
int int2f_11(struct trapframe *);
void int33(struct trapframe *);
void intff(struct trapframe *);

int preserve_int(struct trapframe *);
void setsignal(int, void (*)(struct sigframe *, struct trapframe *));
void spoil_int(void);
void intjmp(int);
void intret(void);

void inb(struct trapframe *, int);
unsigned char inb_traceport(int);
void insb(struct trapframe *, int);
void insx(struct trapframe *, int);
void inx(struct trapframe *, int);
void outb(struct trapframe *, int);
void outb_traceport(int, unsigned char);
void outsb(struct trapframe *, int);
void outsx(struct trapframe *, int);
void outx(struct trapframe *, int);

void done(struct trapframe *, int);
__dead void quit(int) __attribute__((__volatile));
void call_on_quit(void (*)(void *), void *);

void exec_command(int, char *, u_short *, struct trapframe *);
void exec_return(struct trapframe *, int);
int get_env(void);
void load_command(int, char *, char **, char **);

int open_prog(char *);

int squirrel_fd(int);

int i386dis(u_short, u_short, unsigned char *, char *, int);
