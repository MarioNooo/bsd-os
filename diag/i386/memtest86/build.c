/*
 *  linux/tools/build.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * This file builds a disk-image from three different files:
 *
 * - bootsect: max 510 bytes of 8086 machine code, loads the rest
 * - setup: max 4 sectors of 8086 machine code, sets up system parm
 * - test: 80386 code for actual system
 *
 * It does some checking that all files are of the correct type, and
 * just writes the result to stdout, removing headers and padding to
 * the right amount. It also writes some system data to stderr.
 */

/*
 * 1-Jan-96 - Modified by Chris Brady to create a bootable MemTest-86 image.
 * Mostly just ripped out code for root device checks.
 */

#include <stdio.h>	/* fprintf */
#include <string.h>
#include <stdlib.h>	/* contains exit */
#include <sys/types.h>	/* unistd.h needs this */
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>	/* contains read/write */
#include <fcntl.h>
#include <linux/a.out.h>
#include "mtest.h"

#define MINIX_HEADER 32
#define N_MAGIC_OFFSET 1024
static int GCC_HEADER = sizeof(struct exec);

/* max nr of sectors of setup: don't change unless you also change
 * bootsect etc */

#define STRINGIFY(x) #x

typedef union {
	long l;
	short s[2];
	char b[4];
} conv;

long intel_long(long l)
{
	conv t;

	t.b[0] = l & 0xff; l >>= 8;
	t.b[1] = l & 0xff; l >>= 8;
	t.b[2] = l & 0xff; l >>= 8;
	t.b[3] = l & 0xff; l >>= 8;
	return t.l;
}

short intel_short(short l)
{
	conv t;

	t.b[0] = l & 0xff; l >>= 8;
	t.b[1] = l & 0xff; l >>= 8;
	return t.s[0];
}

void die(char * str)
{
	fprintf(stderr,"%s\n",str);
	exit(1);
}

void usage(void)
{
	die("Usage: build bootsect setup");
}

int main(int argc, char ** argv)
{
	int i,c,id, sz;
	unsigned long sys_size;
	char buf[1024];
	struct exec *ex = (struct exec *)buf;
	char major_root, minor_root;
	struct stat sb;

	if (argc < 3) {
		usage();
	}
	for (i=0;i<sizeof buf; i++) buf[i]=0;
	if ((id=open(argv[1],O_RDONLY,0))<0)
		die("Unable to open 'boot'");
	if (read(id,buf,MINIX_HEADER) != MINIX_HEADER)
		die("Unable to read header of 'boot'");
	if (((long *) buf)[0]!=intel_long(0x04100301))
		die("Non-Minix header of 'boot'");
	if (((long *) buf)[1]!=intel_long(MINIX_HEADER))
		die("Non-Minix header of 'boot'");
	if (((long *) buf)[3] != 0)
		die("Illegal data segment in 'boot'");
	if (((long *) buf)[4] != 0)
		die("Illegal bss in 'boot'");
	if (((long *) buf)[5] != 0)
		die("Non-Minix header of 'boot'");
	if (((long *) buf)[7] != 0)
		die("Illegal symbol table in 'boot'");
	i=read(id,buf,sizeof buf);
	fprintf(stderr,"Boot sector %d bytes.\n",i);
	if (i != 512)
		die("Boot block must be exactly 512 bytes");
	i=write(1,buf,512);
	if (i!=512)
		die("Write call failed");
	close (id);
	
	if ((id=open(argv[2],O_RDONLY,0))<0)
		die("Unable to open 'setup'");
	if (read(id,buf,MINIX_HEADER) != MINIX_HEADER)
		die("Unable to read header of 'setup'");
	if (((long *) buf)[0]!=intel_long(0x04100301))
		die("Non-Minix header of 'setup'");
	if (((long *) buf)[1]!=intel_long(MINIX_HEADER))
		die("Non-Minix header of 'setup'");
	if (((long *) buf)[3] != 0)
		die("Illegal data segment in 'setup'");
	if (((long *) buf)[4] != 0)
		die("Illegal bss in 'setup'");
	if (((long *) buf)[5] != 0)
		die("Non-Minix header of 'setup'");
	if (((long *) buf)[7] != 0)
		die("Illegal symbol table in 'setup'");
	for (i=0 ; (c=read(id,buf,sizeof buf))>0 ; i+=c )
		if (write(1,buf,c)!=c)
			die("Write call failed");
	if (c != 0)
		die("read-error on 'setup'");
	close (id);
	if (i > SETUPSECS*512)
		die("Setup exceeds " STRINGIFY(SETUPSECS)
			" sectors - rewrite build/boot/setup");
	fprintf(stderr,"Setup is %d bytes.\n",i);
	for (c=0 ; c<sizeof(buf) ; c++)
		buf[c] = '\0';
	while (i<SETUPSECS*512) {
		c = SETUPSECS*512-i;
		if (c > sizeof(buf))
			c = sizeof(buf);
		if (write(1,buf,c) != c)
			die("Write call failed");
		i += c;
	}
	
	if ((id=open(argv[3],O_RDONLY,0))<0)
		die("Unable to open 'setup'");
#ifndef ELF
	if (read(id,buf,GCC_HEADER) != GCC_HEADER)
		die("Unable to read header of 'test'");
        if (N_MAGIC(*ex) == ZMAGIC) {
                GCC_HEADER = N_MAGIC_OFFSET;
                lseek(id, GCC_HEADER, SEEK_SET);
        } else if (N_MAGIC(*ex) != QMAGIC)
                die("Non-GCC header of 'test'");
	fprintf(stderr,"Test is %d kB (%d kB code, %d kB data and %d kB bss)\n",
		(ex->a_text+ex->a_data+ex->a_bss)/1024,
		ex->a_text /1024,
		ex->a_data /1024,
		ex->a_bss  /1024);
	sz = N_SYMOFF(*ex) - GCC_HEADER + 4;
#else
        if (fstat (id, &sb)) {
          perror ("fstat");
          die ("Unable to stat 'test'");
        }
        sz = sb.st_size;
        fprintf (stderr, "Test is %d kB\n", sz/1024);
	sys_size = (sz + 15) / 16;
	if (sys_size > TSTSIZE)
		die("Test is too big");
#endif
	while (sz > 0) {
		int l, n;

		l = sz;
		if (l > sizeof(buf))
			l = sizeof(buf);
		if ((n=read(id, buf, l)) != l) {
			if (n == -1) 
				perror(argv[1]);
			else
				fprintf(stderr, "Unexpected EOF\n");
			die("Can't read 'test'");
		}
		if (write(1, buf, l) != l)
			die("Write failed");
		sz -= l;
	}

	close(id);
	if (lseek(1,500,0) == 500) {
		buf[0] = (sys_size & 0xff);
		buf[1] = ((sys_size >> 8) & 0xff);
		if (write(1, buf, 2) != 2)
			die("Write failed");
	}
	return(0);
}
