#define DISP(c) write(1, c, strlen(c))

void
_preload_main_(int ver, int nlibs, char **libs,
		 int nmod, char **mods, char *argv0, char **environ)
{
 int i;
 char *p;
 char *p1;
 if (ver != 1)
   {
     DISP("Bad version of ld.so\n");
     return;
   }
 DISP("disinfo: an example of the PRELOAD feature\n");
 DISP("Program name: ");
 if (argv0 && *argv0)
   DISP(argv0);
 else
   DISP("*unknown* (called by ldd?)");
 DISP("\nShared libraries loaded:\n");
 for (i = 0; i < nlibs; i++)
   {
     DISP(libs[i]);
     DISP("\n");
   }
 DISP("Environ:\n");
 for (i = 0; environ[i]; i++)
   {
     DISP(environ[i]);
     DISP("\n");
   }
 DISP("Program arguments:\n");
 /* Well, this is not really beautiful ! */
 for (p = argv0; p != environ[0]; p++)
   {
     DISP(p);
     DISP(" ");
     while (*p) p++;	/* Next arg */
   }
 DISP("\n*** End of dispinfo ***\n");
 return;
}
