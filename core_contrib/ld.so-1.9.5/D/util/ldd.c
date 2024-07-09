/*	BSDI ldd.c,v 1.3 1998/05/06 19:15:26 donn Exp	*/

/*
 * ldd - print shared library dependencies
 *
 * usage: ldd [-vVdr] prog ...
 *        -v: print ldd version
 *        -V: print ld.so version
 *	  -d: Perform relocations and report any missing functions. (ELF only).
 *	  -r: Perform relocations for both data objects and functions, and
 *	      report any missing objects (ELF only).
 *        prog ...: programs to check
 *
 * Copyright 1993-1996, David Engel
 *
 * This program may be used for any purpose as long as this
 * copyright notice is kept.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <a.out.h>
#include <errno.h>
#include <sys/wait.h>
#include <linux/elf.h>
#include "../config.h"

#ifdef __bsdi__
#define	ELFONLY		1
#undef LDDSTUB
#define	LDDSTUB		"/usr/contrib/libexec/lddstub"
#endif

/* see if prog is a binary file */
int is_bin(char *argv0, char *prog)
{
    int res = 0;
    FILE *file;
    struct exec exec;

    /* must be able to open it for reading */
    if ((file = fopen(prog, "rb")) == NULL)
	fprintf(stderr, "%s: can't open %s (%s)\n", argv0, prog,
		strerror(errno));
    else
    {
#ifndef ELFONLY
	/* then see if it's Z, Q or OMAGIC */
	if (fread(&exec, sizeof exec, 1, file) < 1)
	    fprintf(stderr, "%s: can't read header from %s\n", argv0, prog);
	else if (N_MAGIC(exec) != ZMAGIC && N_MAGIC(exec) != QMAGIC &&
                 N_MAGIC(exec) != OMAGIC)
	{
	    struct elfhdr elf_hdr;
	    
 	    rewind(file);
#else
	    struct elfhdr elf_hdr;
#endif
	    fread(&elf_hdr, sizeof elf_hdr, 1, file);
	    if (elf_hdr.e_ident[0] != 0x7f ||
		strncmp(elf_hdr.e_ident+1, "ELF", 3) != 0)
#ifndef ELFONLY
		fprintf(stderr, "%s: %s is not a.out or ELF\n", argv0, prog);
#else
		fprintf(stderr, "%s: %s is not ELF format\n", argv0, prog);
#endif
	    else
	    {
		struct elf_phdr phdr;
		int i;

		/* Check its an exectuable, library or other */
		switch (elf_hdr.e_type)
		{
		  case ET_EXEC:
		    res = 3;
		    /* then determine if it is dynamic ELF */
		    for (i=0; i<elf_hdr.e_phnum; i++)
		    {
		        fread(&phdr, sizeof phdr, 1, file);
			if (phdr.p_type == PT_DYNAMIC)
			{
			    res = 2;
			    break;
			}
		    }
		    break;
		  case ET_DYN:
		    res = 4;
		    break;
		  default:
		    res = 0;
		    break;
		}
	    }
#ifndef ELFONLY
	}
	else
	    res = 1; /* looks good */
#endif

	fclose(file);
    }
    return res;
}

int main(int argc, char **argv, char **envp)
{
    int i;
    int vprinted = 0;
    int resolve = 0;
    int bind = 0;
    int bintype;
    char *ld_preload;
    int status = 0;

#ifndef ELFONLY
    /* this must be volatile to work with -O, GCC bug? */
    volatile loadptr loader = (loadptr)LDSO_ADDR;
#endif
  
    while ((i = getopt(argc, argv, "drvV")) != EOF)
	switch (i)
	{
	case 'v':
	    /* print our version number */
	    printf("%s: version %s\n", argv[0], VERSION);
	    vprinted = 1;
	    break;
	case 'd':
	    bind = 1;
	    break;
	case 'r':
	    resolve = 1;
	    break;
#ifndef ELFONLY
	case 'V':
	    /* print the version number of ld.so */
	    if (uselib(LDSO_IMAGE))
	    {
		fprintf(stderr, "%s: can't load dynamic linker %s (%s)\n",
			argv[0], LDSO_IMAGE, strerror(errno));
		exit(1);
	    }
	    loader(FUNC_VERS, NULL);
	    vprinted = 1;
	    break;
#endif
	default:
	    fprintf(stderr, "usage: ldd [-drv] image ...\n");
	    exit(1);
	}

    /* must specify programs if -v or -V not used */
    if (optind >= argc && !vprinted)
    {
#ifndef ELFONLY
	printf("usage: %s [-vVdr] prog ...\n", argv[0]);
	exit(0);
#else
	fprintf(stderr, "usage: ldd [-drv] image ...\n");
	exit(1);
#endif
    }

    /* setup the environment for ELF binaries */
    putenv("LD_TRACE_LOADED_OBJECTS=1");
    if (resolve || bind)
        putenv("LD_BIND_NOW=1");
    if (resolve)
        putenv("LD_WARN=1");
    ld_preload = getenv("LD_PRELOAD");

    /* print the dependencies for each program */
    for (i = optind; i < argc; i++)
    {
	pid_t pid;
	char buff[1024];

	/* make sure it's a binary file */
	if (!(bintype = is_bin(argv[0], argv[i])))
	{
	    status = 1;
	    continue;
	}

	/* print the program name if doing more than one */
	if (optind < argc-1)
	{
	    printf("%s:\n", argv[i]);
	    fflush(stdout);
	}

	/* no need to fork/exec for static ELF program */
	if (bintype == 3)
	{
#ifndef ELFONLY
	    printf("\tstatically linked (ELF)\n");
#else
	    printf("\tstatically linked\n");
#endif
	    continue;
	}

	/* now fork and exec prog with argc = 0 */
	if ((pid = fork()) < 0)
	{
	    fprintf(stderr, "%s: can't fork (%s)\n", argv[0], strerror(errno));
	    exit(1);
	}
	else if (pid == 0)
	{
	    switch (bintype)
	    {
#ifndef ELFONLY
	      case 1: /* a.out */
	        /* save the name in the enviroment, ld.so may need it */
	        snprintf(buff, sizeof buff, "%s=%s", LDD_ARGV0, argv[i]);
		putenv(buff);
		execl(argv[i], NULL);
		break;
#endif
	      case 2: /* ELF program */
		execl(argv[i], argv[i], NULL);
		break;
	      case 4: /* ELF library */
#ifndef __bsdi__
		/* try to use /lib/ld-linux.so.2 first */
		execl("/lib/ld-linux.so.2", "/lib/ld-linux.so.2", 
		      "--list", argv[i], NULL);
#endif
	        /* if that fials, add library to LD_PRELOAD and 
		   then execute lddstub */
		if (ld_preload && *ld_preload)
		    snprintf(buff, sizeof buff, "%s=%s:%s%s", "LD_PRELOAD",
			     ld_preload, *argv[i] == '/' ? "" : "./", argv[i]);
		else
		    snprintf(buff, sizeof buff, "%s=%s%s", "LD_PRELOAD", 
			     *argv[i] == '/' ? "" : "./", argv[i]);
		putenv(buff);
		execl(LDDSTUB, argv[i], NULL);
		break;
	      default:
		fprintf(stderr, "%s: internal error, bintype = %d\n",
			argv[0], bintype);
		exit(1);
	    }
	    fprintf(stderr, "%s: can't execute %s (%s)\n", argv[0], argv[i],
		    strerror(errno));
	    exit(1);
	}
	else
	{
	    /* then wait for it to complete */
	    int status;
	    if (waitpid(pid, &status, 0) != pid)
	    {
	        fprintf(stderr, "%s: error waiting for %s (%s)\n", argv[0], 
			argv[i], strerror(errno));
	    }
	    else if (WIFSIGNALED(status))
	    {
	        fprintf(stderr, "%s: %s exited with signal %d\n", argv[0], 
			argv[i], WTERMSIG(status));
	    }
	}
    }

    exit(status);
}
