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
 *	BSDI doscmd.c,v 2.6 2003/07/08 21:56:55 polk Exp
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <machine/psl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <limits.h>

#include "doscmd.h"

FILE *debugf;
int  capture_fd = -1;
int  dead = 0;
int  intnum;
int  xmode = 0;
int  dosmode = 0;
int  use_fork = 0;
int  tflag = 0;
int  ecnt = 0;
int booting = 0;
int raw_kbd = 0;
char *envs[256];
unsigned long *ivec = (unsigned long *)0;
struct vconnect_area vconnect_area = {
	0,				/* Interrupt state */
	PRB_V86_FORMAT,			/* Magic number */
	{ 0, },				/* Pass through ints */
	{ 0xf1000000, 0xf1000010 }	/* Magic iret location */
};

int disassemble(struct trapframe *, char *);

void
usage ()
{
	fprintf (stderr, "usage: doscmd cmd args...\n");
	quit (1);
}

#define	BPW	(sizeof(u_long) << 3)
u_long debug_ints[256/BPW];

inline void
debug_set(int x)
{
    x &= 0xff;
    debug_ints[x/BPW] |= 1 << (x & (BPW - 1));
}

inline void
debug_unset(int x)
{
    x &= 0xff;
    debug_ints[x/BPW] &= ~(1 << (x & (BPW - 1)));
}

inline u_long
debug_isset(int x)
{
    x &= 0xff;
    return(debug_ints[x/BPW] & (1 << (x & (BPW - 1))));
}

char *dos_path = 0;
char cmdname[256];

char *memfile = "/tmp/doscmd.XXXXXX";

int
main (int argc, char **argv, char **envp)
{
	struct trapframe tf;
	struct sigframe sf;
	int fd = -1;
	int i;
	int c;
	extern int optind;
	char prog[1024];
    	char buffer[4096];
	int zflag = 0;
    	int mfd;
	FILE *fp;
    	char *col;
    	int p;

	exec_level = 0;

	debug_flags = D_ALWAYS;
	debugf = stderr;

	fd = open ("/dev/null", O_RDWR);
	if (fd != 3)
		dup2 (fd, 3); /* stdaux */
	if (fd != 4)
		dup2 (fd, 4); /* stdprt */
	if (fd != 3 && fd != 4)
		close (fd);

    	fd = -1;

    	debug_set(0);		/* debug any D_TRAPS without intnum */

	while ((c = getopt (argc, argv, "234Oc:IELMPRAU:S:HDTtzvVxXfbri:o:d:")) != EOF) {
		switch (c) {
		case 'd':
			if (fp = fopen(optarg, "w")) {
				debugf = fp;
				setbuf (fp, NULL);
			} else
				perror(optarg);
			break;
		case '2':
			debug_flags |= D_TRAPS2;
			break;
		case '3':
			debug_flags |= D_TRAPS3;
			break;
		case '4':
			debug_flags |= D_DEBUGIN;
			break;
    	    	case 'O':
		    	debugf = stdout;
			setbuf (stdout, NULL);
		    	break;
    	    	case 'c':
			if ((capture_fd = creat(optarg, 0666)) < 0) {
		    	    perror(optarg);
			    quit(1);
			}
			break;
	    	case 'i':
			i = 1;
			if (col = strchr(optarg, ':')) {
			    *col++ = 0;
			    i = strtol(col, 0, 0);
			}
			p = strtol(optarg, 0, 0);

			while (i-- > 0)
			    define_input_port_handler(p++, inb_traceport);
			break;
	    	case 'o':
			i = 1;
			if (col = strchr(optarg, ':')) {
			    *col++ = 0;
			    i = strtol(col, 0, 0);
			}
			p = strtol(optarg, 0, 0);

			while (i-- > 0)
			    define_output_port_handler(p++, outb_traceport);
			break;

		case 'r':
			raw_kbd = 1;
			break;
		case 'X':
			raw_kbd = 2;
			break;
    	    	case 'I':
			debug_flags |= D_ITRAPS;
			for (c = 0; c < 256; ++c)
				debug_set(c);
			break;
    	    	case 'E':
			debug_flags |= D_EXEC;
			break;
    	    	case 'M':
			debug_flags |= D_MEMORY;
			break;
		case 'P':
			debug_flags |= D_PORT;
			break;
		case 'R':
			debug_flags |= D_REDIR;
			break;
		case 'L':
			debug_flags |= D_PRINTER;
			break;
		case 'A':
			debug_flags |= D_TRAPS|D_ITRAPS;
			for (c = 0; c < 256; ++c)
				debug_set(c);
			break;
		case 'U':
			debug_unset(strtol(optarg, 0, 0));
			break;
		case 'S':
			debug_flags |= D_TRAPS|D_ITRAPS;
			debug_set(strtol(optarg, 0, 0));
			break;
		case 'H':
			debug_flags |= D_HALF;
			break;
		case 'x':
			xmode = 1;
			break;
		case 'T':
			debug_flags |= D_TEMP;
			break;
    	    	case 'f':
			use_fork = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'z':
			zflag = 1;
			break;
    	    	case 'D':
			debug_flags |= D_DISK | D_FILE_OPS;
			break;
		case 'v':
			debug_flags |= D_TRAPS | D_ITRAPS | D_HALF | 0xff;
			break;
		case 'V':
			vflag = 1;
			break;
		case 'b':
			booting = 1;
			break;
		default:
			usage ();
		}
	}

    	if (xmode && use_fork) {
		fprintf(stderr, "Cannot both enable fork and X\n");
		quit(1);
    	}

    	if (dosmode && use_fork) {
		fprintf(stderr, "Cannot both enable fork and dos\n");
		quit(1);
    	}

	if (vflag && debugf == stderr) {
		debugf = stdout;
		setbuf (stdout, NULL);
	}

	mfd = mkstemp(memfile);

	if (mfd < 0) {
		fprintf(stderr, "memfile: %s\n", strerror(errno));
		fprintf(stderr, "High memory will not be mapped\n");
	} else {
	    caddr_t add;

	    unlink(memfile);

    	    mfd = squirrel_fd(mfd);

	    lseek(mfd, 64 * 1024 - 1, 0);
	    write(mfd, "", 1);
	    add = mmap((caddr_t)0x000000, 64 * 1024,
			PROT_EXEC | PROT_READ | PROT_WRITE,
			MAP_FILE | MAP_FIXED | MAP_INHERIT | MAP_SHARED,
			mfd, 0);
	    add = mmap((caddr_t)0x100000, 64 * 1024,
			PROT_EXEC | PROT_READ | PROT_WRITE,
			MAP_FILE | MAP_FIXED | MAP_INHERIT | MAP_SHARED,
			mfd, 0);
	}
	
	mem_init();

    	if (raw_kbd) {
	    console_init();
    	}
	init_devinit_handlers();

	/*
	 * initialize all port handlers for INB, OUTB, ...
	 */
	init_io_port_handlers();

	if ((fp = fopen(".doscmdrc", "r")) == NULL) {
	    struct passwd *pwd = getpwuid(geteuid());
	    if (pwd) {
		sprintf(buffer, "%s/.doscmdrc", pwd->pw_dir);
		fp = fopen(buffer, "r");
	    }
	    if (!fp)
		fp = fopen("/etc/doscmdrc", "r");
	}

    	/*
	 * With no arguments we will assume we must boot dos
	 */
	if (optind >= argc) {
#if defined(VER11)
    	    if (!booting) {
		fprintf(stderr, "Usage: doscmd cmd [args]\n");
		quit(1);
	    }
#endif
	    booting = 1;
    	}
#if defined(VER11)
    	if (booting || xmode) {
	    fprintf(stderr, "Warning: You are using a not yet supported or documented feature of doscmd.\n");
	    fprintf(stderr, "       : Proceed at your own risk.\n");
	    fprintf(stderr, "       : Do not cry out for help.\n");
	}
#endif
    	if (booting) {
	    /*
	     * I would like to let INT 2F pass through as well, but I
	     * need to get my hands on INT 2F:11 to do file redirection.
	     *
	     */

    	    for (i = 0; i < 0xff; ++i) {
	    	switch (i) {
		case 0x09:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x1a:
		case 0x20:
		case 0x2f:
		case 0x33:
		case 0xfe:
		case 0xff:
		    vconnect_area.passthru[i >> 5] &= ~(1 << (i & 0x1f));
		    break;
		case 0x10:
		    if (raw_kbd)
			vconnect_area.passthru[i >> 5] |= 1 << (i & 0x1f);
    	    	    else
			vconnect_area.passthru[i >> 5] &= ~(1 << (i & 0x1f));
		    break;
    	    	case 0x21:
    	    	case 0x2a:
    	    	case 0x29:
    	    	case 0x28:
			vconnect_area.passthru[i >> 5] |= 1 << (i & 0x1f);
		    break;
    	    	default:
if (0)
		    vconnect_area.passthru[i >> 5] |= 1 << (i & 0x1f);
		    break;
    	    	}
    	    }
    	}

    	if (raw_kbd == 2)
		goto skip_this;

    	if (fp) {
	    if (booting) {
		booting = read_config(fp);
    	    	if (booting < 0) {
		    if ((fd = disk_fd(booting = 0)) < 0)	/* A drive */
			fd = disk_fd(booting = 2);		/* C drive */
    	    	} else {
		    fd = disk_fd(booting);
    	    	}

    	    	if (fd < 0) {
		    fprintf(stderr, "Cannot boot from %c: (can't open)\n",
				     booting + 'A');
		    quit(1);
    	    	}
    	    } else
		read_config(fp);
    	} else if (booting) {
	    fprintf(stderr, "You must have a doscmdrc to boot\n");
	    quit(1);
	}

    	if (fd >= 0) {
	    if (read(fd, (char *)0x7c00, 512) != 512) {
		fprintf(stderr, "Short read on boot block from %c:\n",
			        booting + 'A');
		quit(1);
	    }
skip_this:
	    dosmode = 1;
	    init_cs = 0x0000;
	    init_ip = 0x7c00;

	    init_ds = 0x0000;
	    init_es = 0x0000;
	    init_ss = 0x9800;
	    init_sp = 0x8000 - 2;
	} else {
	    if (optind >= argc)
		    usage ();

	    if (dos_getcwd('C' - 'A') == NULL) {
		char *p;

		p = getcwd(buffer, sizeof(buffer));
		if (!p || !*p) p = getenv("PWD");
		if (!p || !*p) p = "/";
		init_path('C' - 'A', (u_char *)"/", (u_char *)p);

		for (i = 0; i < ecnt; ++i) {
		    if (!strncmp(envs[i], "PATH=", 5)) {
			dos_path = envs[i] + 5;
			break;
		    }
		}
		if (i >= ecnt) {
		    static char path[256];
		    sprintf(path, "PATH=C:%s", dos_getcwd('C' - 'A'));
		    put_dosenv(path);
		    dos_path = envs[ecnt-1] + 5;
		}
	    }

	    for (i = 0; i < ecnt; ++i) {
		if (!strncmp(envs[i], "COMSPEC=", 8))
		    break;
	    }
	    if (i >= ecnt)
	    	put_dosenv("COMSPEC=C:\\COMMAND.COM");

	    for (i = 0; i < ecnt; ++i) {
		if (!strncmp(envs[i], "PATH=", 5)) {
		    dos_path = envs[i] + 5;
		    break;
		}
	    }
	    if (i >= ecnt) {
	    	put_dosenv("PATH=C:\\");
		dos_path = envs[ecnt-1] + 5;
	    }

	    for (i = 0; i < ecnt; ++i) {
		if (!strncmp(envs[i], "PROMPT=", 7))
		    break;
	    }
	    if (i >= ecnt)
	    	put_dosenv("PROMPT=DOS> ");

	    envs[ecnt] = 0;

	    if (dos_getcwd('R' - 'A') == NULL)
		init_path('R' - 'A', (u_char *)"/", 0);

	    strncpy(prog, argv[optind++], sizeof(prog) -1);
	    prog[sizeof(prog) -1] = '\0';

	    if ((fd = open_prog (prog)) < 0) {
		    fprintf (stderr, "%s: command not found\n", prog);
		    quit (1);
	    }
	    load_command (fd, cmdname, argv + optind, envs);
	}

	setsignal (SIGSEGV, random_sig_handler);
	setsignal (SIGFPE, floating);		/**/

	setsignal (SIGBUS, trap);		/**/
	setsignal (SIGILL, trap);		/**/
	setsignal (SIGILL, sigill);		/**/
	setsignal (SIGTRAP, sigtrap);		/**/
	setsignal (SIGUSR2, sigtrace);		/**/

	video_init();
	video_bios_init();
	cmos_init();
    	bios_init();
    	speaker_init();

	init_optional_devices();

	gettimeofday(&boot_time, 0);

    	if (xmode || raw_kbd == 1)
	    timer_init();

	if (zflag) for (;;) pause();

	tf.tf_ax = 0;
	tf.tf_bx = 0;
	tf.tf_cx = 0;
	tf.tf_dx = 0;
	tf.tf_si = 0;
	tf.tf_di = 0;
	tf.tf_bp = 0;

    	if (raw_kbd) {
	    /*
    	     * If we have a raw keyboard, and hence, video,
	     * sneak in a call to the video BIOS to reinit the
	     * the video display.
	     */
	    static u_char icode[] = {
#if 0
		 0xB8, 0x00, 0x05,	/* mov ax,00500h */
		 0xCD, 0x10,		/* int 010h */
		 0xB8, 0x14, 0x11,	/* mov ax,01114h */
		 0x83, 0xF3, 0x00,	/* xor bx,0 */
		 0xCD, 0x10,		/* int 010h */
		 0xB8, 0x00, 0x12,	/* mov ax,01200h */
		 0xBB, 0x36, 0x00,	/* mov bx,00036h */
		 0xCD, 0x10,		/* int 010h */
#endif
		 0xB8, 0x03, 0x00,	/* mov ax,00003h */
		 0xCD, 0x10,		/* int 010h */
    	    };

	    u_char *ip = (u_char *)0x7c00 - sizeof(icode);
	    memcpy(ip, icode, sizeof(icode));
	    init_ip -= sizeof(icode);

	    /*
    	     * int 0xfe is the magical quit interrupt
	     */
	    if (raw_kbd == 2) {
		ip += sizeof(icode);
		*ip++ = 0xcd;
		*ip++ = 0xfe;
	    }
    	}
	

	tf.tf_eflags = 0x20202;
	tf.tf_cs = init_cs;
	tf.tf_ip = init_ip;
	tf.tf_ss = init_ss;
	tf.tf_sp = init_sp;

	tf.tf_ds = init_ds;
	tf.tf_es = init_es;


	sf.sf_eax = (booting || raw_kbd) ? (int)&vconnect_area : -1;
	sf.sf_scp = &sf.sf_sc;
	sf.sf_sc.sc_ps = PSL_VM;
	sigemptyset(&sf.sf_sc.sc_mask);
	sf.sf_sc.sc_onstack = 0;

    	if (tflag) {
		tmode = 1;
		tracetrap(&tf);
    	}

	_switch_vm86 (sf, tf);

	if (vflag) dump_regs(&tf);
	fatal ("vm86 returned\n");
}

int
open_name(char *name, char *ext)
{
    int fd;
    char *p = name;
    while (*p)
	++p;

    *ext = 0;
    if (!strstr(name, ".exe") && !strstr(name, ".com")) {
	    strcpy(ext, ".exe");
	    strcpy(p, ".exe");
    }
    if ((fd = open (name, O_RDONLY)) >= 0) {
	    return (fd);
    }

    *p = 0;
    if (!strstr(name, ".exe") && !strstr(name, ".com")) {
	    strcpy(ext, ".com");
	    strcpy(p, ".com");
    }
    if ((fd = open (name, O_RDONLY)) >= 0) {
	    return (fd);
    }
    return(-1);
}

int
open_prog (name)
char *name;
{
	int fd;
	char *fullname;
	char *dirs;
	char *p;
	char *e;
	int end_flag;
    	char ext[5];


	dirs = dos_path;

    	if (name[1] == ':' || name[0] == '/' || name[0] == '\\'
			   || name[0] == '.') {
	    fullname = translate_filename(name);
	    fd = open_name(fullname, ext);

	    strcpy(cmdname, name);
    	    if (*ext)
		strcat(cmdname, ext);
	    return(fd);
	}

	fullname = alloca (strlen (dos_path) + strlen (name) + 10);

	end_flag = 0;

	while (*dirs) {
	    p = dirs;
	    while (*p && *p != ';')
		++p;

	    strncpy(fullname, dirs, p - dirs);
	    fullname[p-dirs] = '\0';
	    e = fullname + (p - dirs);
	    dirs = *p ? p + 1 : p;

	    *e++ = '\\';
	    strcpy(e, name);

	    e = translate_filename(fullname);

	    fd = open_name(e, ext);
	    if (fd >= 0) {
		strcpy(cmdname, fullname);
		if (*ext)
		    strcat(cmdname, ext);
		return(fd);
	    }
	}

	return (-1);
}

void
dump_regs (tf)
struct trapframe *tf;
{
	u_char *addr;
	int i;
	char buf[100];
	debug (D_ALWAYS, "\n");
	debug (D_ALWAYS, "ax=%04x bx=%04x cx=%04x dx=%04x\n",
	       tf->tf_ax, tf->tf_bx, tf->tf_cx, tf->tf_dx);
	debug (D_ALWAYS, "si=%04x di=%04x sp=%04x bp=%04x\n",
	       tf->tf_si, tf->tf_di, tf->tf_sp, tf->tf_bp);
	debug (D_ALWAYS, "cs=%04x ss=%04x ds=%04x es=%04x\n",
		 tf->tf_cs, tf->tf_ss, tf->tf_ds, tf->tf_es);
	debug (D_ALWAYS, "ip=%x flags=%x\n", tf->tf_ip, tf->tf_eflags);

	addr = (u_char *)MAKE_ADDR (tf->tf_cs, tf->tf_ip);

	for (i = 0; i < 16; i++)
		debug (D_ALWAYS, "%02x ", addr[i]);
	debug (D_ALWAYS, "\n");

	disassemble (tf, buf);
	debug (D_ALWAYS, "%s\n", buf);
}

#include <stdarg.h>

void
debug (int flags, char *fmt, ...)
{
	va_list args;

	if (flags & (debug_flags & ~0xff)) {
		if ((debug_flags & 0xff) == 0
		    && (flags & (D_ITRAPS|D_TRAPS))
		    && !debug_isset(flags & 0xff))
			return;
		va_start (args, fmt);
		vfprintf (debugf, fmt, args);
		va_end (args);
	}
}

void
fatal (char *fmt, ...)
{
	va_list args;

	dead = 1;

    	if (xmode) {
		char buf[1024];
		char *m;

		va_start (args, fmt);
		vfprintf (debugf, fmt, args);
		vsprintf (buf, fmt, args);
		va_end (args);

		tty_move(23, 0);
		for (m = buf; *m; ++m)
			tty_write(*m, 0x0400);

		tty_move(24, 0);
		for (m = "(PRESS <CTRL-ALT> ANY MOUSE BUTTON TO exit)"; *m; ++m)
			tty_write(*m, 0x0900);
		tty_move(-1, -1);
		for (;;)
			tty_pause();
	}

	va_start (args, fmt);
	fprintf (debugf, "doscmd: fatal error ");
	vfprintf (debugf, fmt, args);
	va_end (args);
	quit (1);
}

void
done (tf, val)
struct trapframe *tf;
int val;
{
    	if (curpsp < 2  && xmode) {
	    char *m;

	    tty_move(24, 0);
	    for (m = "END OF PROGRAM"; *m; ++m)
		    tty_write(*m, 0x8400);

	    for (m = "(PRESS <CTRL-ALT> ANY MOUSE BUTTON TO exit)"; *m; ++m)
		    tty_write(*m, 0x0900);
	    tty_move(-1, -1);
	    for (;;)
		    tty_pause();
	}
	if (!use_fork)
	    exec_return(tf, val);
    	if (use_fork || !curpsp)
	    quit (val);
}

/* returns instruction length */
int
disassemble (tf, buf)
struct trapframe *tf;
char *buf;
{
	int addr;

	addr = (int)MAKE_ADDR (tf->tf_cs, tf->tf_ip);
	return (i386dis (tf->tf_cs, tf->tf_ip, (u_char *)addr, buf, 0));
}

void
put_dosenv(char *value)
{
    if (ecnt < sizeof(envs)/sizeof(envs[0])) {
    	if ((envs[ecnt++] = strdup(value)) == NULL) {
	    perror("put_dosenv");
	    quit(1);
	}
    } else {
	fprintf(stderr, "Environment full, ignoring %s\n", value);
    }
}

int
squirrel_fd(int fd)
{
    int sfd = sysconf(_SC_OPEN_MAX);
    struct stat sb;

    do {
	errno = 0;
    	fstat(--sfd, &sb);
    } while (sfd > 0 && errno != EBADF);

    if (errno == EBADF && dup2(fd, sfd) >= 0) {
	close(fd);
	return(sfd);
    }
    return(fd);
}

int
booted()
{
	return(booting);
}

void
unknown_int2(int maj, int min, struct trapframe *tf)
{
    if (vflag) dump_regs(tf);
    printf("Unknown interrupt %02x function %02x\n", maj, min);
    tf->tf_eflags |= PSL_C;
#if defined(VER11)
    if (!booting)
	quit(1);
#endif
}

void
unknown_int3(int maj, int min, int sub, struct trapframe *tf)
{
    if (vflag) dump_regs(tf);
    printf("Unknown interrupt %02x function %02x subfunction %02x\n",
	   maj, min, sub);
    tf->tf_eflags |= PSL_C;
#if defined(VER11)
    if (!booting)
	quit(1);
#endif
}

void
unknown_int4(int maj, int min, int sub, int ss, struct trapframe *tf)
{
    if (vflag) dump_regs(tf);
    printf("Unknown interrupt %02x function %02x subfunction %02x %02x\n",
	   maj, min, sub, ss);
    tf->tf_eflags |= PSL_C;
#if defined(VER11)
    if (!booting)
	quit(1);
#endif
}

typedef struct COQ {
    void 	(*func)();
    void	*arg;
    struct COQ	*next;
} COQ;

COQ *coq = 0;

void
quit(int status)
{
    while (coq) {
	COQ *c = coq;
	coq = coq->next;
	c->func(c->arg);
    }
    exit(status);
}

void
call_on_quit(void (*func)(void *), void *arg)
{
    COQ *c = (COQ *)malloc(sizeof(COQ));
    if (!c) {
	perror("call_on_quit");
    	quit(1);
    }
    c->func = func;
    c->arg = arg;
    c->next = coq;
    coq = c;
}
