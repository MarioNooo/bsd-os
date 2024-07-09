#include "sieve_lock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


LockStruct
ObtainLock(path, lfile)
     char *path;
     char *lfile;
{
  LockStruct l;
  int trys;
  int fd;
  struct stat statbuf;
  char randstr[20];
  char pid_buf[20];
  char new_path[256];

  srand(time(NULL));
  l.result=0;

  sprintf(pid_buf, "%lu", getpid());
  
  strncpy(new_path, path, sizeof(new_path));
  new_path[sizeof(new_path)-1] = 0;
  if (new_path[strlen(new_path)-1]=='/') {
    new_path[strlen(new_path)-1] = 0;
  }
  if (strlen(new_path)+strlen(lfile)>=sizeof(l.lockfile)) {
    return l;
  }
  sprintf(l.lockfile, "%s/%s", new_path, lfile);
  free(new_path);

  for (trys=0; trys<3; trys++) {
    sprintf(randstr, ".tmp_sievead_%u", rand());
    if (strlen(new_path)+strlen(randstr)>=sizeof(l.tempfile)) {
      return l;
    }
    sprintf(l.tempfile, "%s/%s", new_path, randstr);

    if ((fd=creat(l.tempfile, 0600))>=0) {
      write(fd, pid_buf, strlen(pid_buf));
      close(fd);
      if (!link(l.tempfile, l.lockfile)) {
	unlink(l.tempfile);
	l.result=1;
	return l;
      }
      else {
	unlink(l.tempfile);

	if (!stat(l.lockfile, &statbuf)) {
	  if (statbuf.st_mtime<time(NULL)-5*60) {
	    if(!unlink(l.lockfile)) {
	      trys--;
	      continue;
	    }
	  }
	}
      }
    }
    sleep(1);
  }

  return l;
}

void
ReleaseLock(l)
     LockStruct l;
{
  unlink(l.lockfile);
}


