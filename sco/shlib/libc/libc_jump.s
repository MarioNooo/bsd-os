/*	BSDI libc_jump.s,v 2.2 1997/10/31 03:14:02 donn Exp	*/

/*
 * iBCS2 shared C library jump table.
 * From iBCS2 p 6-3 through p 6-6...
 */

	.section ".text"

	.org 0x0000
	jmp sco__doprnt

	.org 0x0005
	jmp sco__bufsync

	.org 0x000a
	jmp sco_cerror

	.org 0x000f
	jmp sco__cleanup

	.org 0x0014
	jmp sco__filbuf

	.org 0x0019
	jmp sco__findbuf

	.org 0x001e
	jmp sco__findiop

	.org 0x0023
	jmp sco__flsbuf

	.org 0x0028
	jmp sco__wrtchk

	.org 0x002d
	jmp sco__xflsbuf

	.org 0x0032
	jmp abs

	.org 0x0037
	jmp sco_access

	.org 0x003c
	jmp atof

	.org 0x0041
	jmp atoi

	.org 0x0046
	jmp atol

	.org 0x004b
	jmp sco_brk

	.org 0x0050
	jmp calloc

	.org 0x0055
	jmp cfree

	.org 0x005a
	jmp sco_chdir

	.org 0x005f
	jmp sco_chmod

	.org 0x0064
	jmp sco_close

	.org 0x0069
	jmp sco_creat

	.org 0x006e
	jmp ecvt

	.org 0x0073
	jmp sco_fclose

	.org 0x0078
	jmp sco_fcntl

	.org 0x007d
	jmp fcvt

	.org 0x0082
	jmp sco_fflush

	.org 0x0087
	jmp sco_fgetc

	.org 0x008c
	jmp sco_fgets

	.org 0x0091
	jmp sco_fopen

	.org 0x0096
	jmp sco_fprintf

	.org 0x009b
	jmp sco_fputc

	.org 0x00a0
	jmp sco_fputs

	.org 0x00a5
	jmp sco_fread

	.org 0x00aa
	jmp free

	.org 0x00af
	jmp sco_freopen

	.org 0x00b4
	jmp frexp

	.org 0x00b9
	jmp sco_fseek

	.org 0x00be
	jmp sco_fstat

	.org 0x00c3
	jmp sco_fwrite

	.org 0x00c8
	jmp gcvt

	.org 0x00cd
	jmp sco_getchar

	.org 0x00d2
	jmp sco_getenv

	.org 0x00d7
	jmp getopt

	.org 0x00e1
	jmp sco_gets

	.org 0x00e6
	jmp sco_getw

	.org 0x00eb
	jmp sco_ioctl

	.org 0x00f0
	jmp isatty

	.org 0x00f5
	jmp isnan

	.org 0x00fa
	jmp sco_kill

	.org 0x00ff
	jmp ldexp

	.org 0x0104
	jmp sco_lseek

	.org 0x0109
	jmp malloc

	.org 0x010e
	jmp memccpy

	.org 0x0113
	jmp memchr

	.org 0x0118
	jmp memcmp

	.org 0x011d
	jmp memcpy

	.org 0x0122
	jmp memset

	.org 0x0127
	jmp mktemp

	.org 0x012c
	jmp sco_open

	.org 0x0131
	jmp sco_printf

	.org 0x0136
	jmp sco_putchar

	.org 0x013b
	jmp sco_puts

	.org 0x0140
	jmp sco_putw

	.org 0x0145
	jmp sco_read

	.org 0x014a
	jmp realloc

	.org 0x014f
	jmp sco_sbrk

	.org 0x0154
	jmp sco_setbuf

	.org 0x0159
	jmp sco_sighold

	.org 0x015e
	jmp sco_sigignore

	.org 0x0163
	jmp sco_signal

	.org 0x0168
	jmp sco_sigpause

	.org 0x016d
	jmp sco_sigrelse

	.org 0x0172
	jmp sco_sigset

	.org 0x0177
	jmp sprintf

	.org 0x017c
	jmp sco_stat

	.org 0x0181
	jmp strcat

	.org 0x0186
	jmp strchr

	.org 0x018b
	jmp strcmp

	.org 0x0190
	jmp strcpy

	.org 0x0195
	jmp strlen

	.org 0x019a
	jmp strncat

	.org 0x019f
	jmp strncmp

	.org 0x01a4
	jmp strncpy

	.org 0x01a9
	jmp strrchr

	.org 0x01ae
	jmp sco_time

	.org 0x01b3
	jmp tolower

	.org 0x01b8
	jmp toupper

	.org 0x01bd
	jmp sco_ungetc

	.org 0x01c2
	jmp sco_unlink

	.org 0x01c7
	jmp sco_utime

	.org 0x01cc
	jmp sco_write

	.org 0x02e9
	jmp sco_getpid
