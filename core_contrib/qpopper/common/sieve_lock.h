#ifndef _FILELOCK_H
#define _FILELOCK_H

#include "config.h"

typedef struct {
  char lockfile[256];
  char tempfile[256];
  int result;
} LockStruct;


LockStruct
ObtainLock __PROTO((char *path, char *lfile));

void
ReleaseLock __PROTO((LockStruct l));

#endif
