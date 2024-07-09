/*
 * POSIX semaphores
 */

/*
 * Named semaphores:
 *     Can be shared between processes only when they start with a '/'.
 *     Forward slashes in name are converted to underscores
 *     Names are converted to files like:
 *         /name     -->  /tmp/.sem_name
 *         /foo/bar  -->  /tmp/.sem_foo_bar
 *     Names that aren't absolute include the pid
 *         foo/bar   -->  /tmp/.sem12345foo_bar
 *
 * Advisory locks are used since the semaphores are cooperative and a 
 * process might use a resource without a semaphore.  So there is no need
 * for a more restrictive method.
 *
 * There is no underlying file for unnamed semaphores.  In addition, we
 * do not support the pshared option for sem_init.  If processes wish to
 * share a semaphore, they can agree to an absolute named one.
 * Supporting the pshared option will require help from the kernel in
 * locking the semaphore's data during access.
 */

#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>

#ifndef TRUE
#define FALSE (0)
#define TRUE (!FALSE)
#endif

#include "thread_private.h"

/*
 * Storage space for named semaphores
 */
static sem_t _named_semaphores[SEM_NSEMS_MAX];

/* A mutex for threaded access to above array */
static pthread_mutex_t _named_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int errno;

/* We need to set up atexit call when we deal with named semaphores */
static int atexit_setup = FALSE;


/*
 * Returns a malloced cleaned up version of name which can be used
 * as a file name with above rules.  On error returns NULL and has
 * errno set.
 */
static char *
_convert_name(name)
	const char *name;
{
	const char beginning[] = "/tmp/.sem";
	char *fullname = NULL;
	int fullname_len;
	char pid_str[6];		/* If using pid in name */
	char *p = NULL;
	char *dest = NULL;
	char c;

	fullname_len = strlen(beginning);

	/* If not an absolute name, include pid */
	if (name[0] != '/') {
		snprintf(pid_str, 6, "%05d", (uint) getpid());
		fullname_len += 5;		/* Always 5 characters */
	}

	fullname_len += strlen(name);

	/* If name is too long, better to catch it here */
	if (fullname_len > FILENAME_MAX) {
		errno = ENAMETOOLONG;
		return(NULL);
	}

	fullname = malloc(fullname_len + 1);
	if (fullname == NULL)
		return(NULL);

	strcpy(fullname, beginning);
	if (name[0] != '/')
		(void) strcat(fullname, pid_str);
	dest = strchr(fullname, '\0');

	/* Walk the name and convert forward slashes */
	p = (char *) name;
	while ((c = *(p++)) != '\0')
		*(dest++) = (c == '/') ? '_' : c;

	/* Sanity - do not allow normal error trapping... */
	assert (strlen(fullname) == fullname_len);

	return(fullname);
}


/*
 * Get data from a locked semaphore.  Caller has already checked that
 * this is a valid semaphore.
 */
static int
_get_semaphore(sem, value)
	sem_t	*sem;
	int	*value;
{
	if (sem->name) {
		if (lseek(sem->fd, 0, SEEK_SET) == -1)
			return(-1);
		if (read(sem->fd, value, sizeof(int)) != sizeof(int))
			return(-1);
	} else
		*value = sem->value;

	return(0);
}


/*
 * Store data to a locked semaphore.  Caller has already checked that
 * this is a valid semaphore.
 */
static int
_store_semaphore(sem, value)
	sem_t   *sem;
	int     value;
{
	if (sem->name) {
		if (lseek(sem->fd, 0, SEEK_SET) == -1)
			return(-1);
		if (write(sem->fd, &value, sizeof(int)) != sizeof(int))
			return(-1);
	} else
		sem->value = value;

	return(0);
}


/*
 * Lock a semaphore.  Normally a process blocks waiting, but shouldn't
 * do that with threads.  The tiny wait should tell the thread scheduler
 * to move on to another thread so we don't waste the CPU.  Caller has
 * already checked that this is a valid semaphore, even if sem->magic
 * is not SEM_MAGIC (during sem_open()).  Signals have to be blocked as
 * a signal handler could call sem_post() while we are doing another
 * semaphore operation.
 */
static int
_lock_semaphore(sem)
	sem_t *sem;
{
	int flags = LOCK_EX | LOCK_NB;
	int status = 0;
	sigset_t allsig;

	/* Block signals */
	sigfillset(&allsig);
	if (sigprocmask(SIG_SETMASK, &allsig, &sem->original_mask) == -1)
		return(-1);

	/* We are now single threaded, no special work for unnamed semaphore */

	if (sem->name) {
		sem->fd = open(sem->name, O_RDWR);
		if (sem->fd < 0)
			goto fail;
		do {
			status = flock(sem->fd, flags);
			if (status < 0) {
				if (errno != EWOULDBLOCK)
					goto fail;

				/* Would block, so wait */
				(void) sigprocmask(SIG_SETMASK,
				    &sem->original_mask, NULL);
				usleep(1000);		/* Tiny wait */
				if (sigprocmask(SIG_SETMASK, &allsig,
				    &sem->original_mask) == -1)
					return(-1);
			}
		} while (status == -1);
	}

	/* Signals remain blocked */

	return(0);

fail:
	status = errno;
	if (sem->fd != -1)
		(void) close(sem->fd);

	/* Restore signals */
	(void) sigprocmask(SIG_SETMASK, &sem->original_mask, NULL);

	errno = status;
	return(-1);
}


/*
 * Unlock a semaphore.  Caller has already checked that this was a valid
 * semaphore, even though sem->magic might not equal SEM_MAGIC.
 * Any errors here are ignored as not much can be done.
 */
static void
_unlock_semaphore(sem)
	sem_t *sem;
{
	/* We are single threaded */

	if (sem->name) {
		(void) flock(sem->fd, LOCK_UN);
		(void) close(sem->fd);
		sem->fd = -1;		/* Paranoia */
	}

	/* Restore signals */
	(void) sigprocmask(SIG_SETMASK, &sem->original_mask, NULL);
}


/*
 * During forced closing, close all absolute named semaphores properly.
 * Unlink any named semaphores that are relatively named.
 */
static void
_sem_cleanup()
{
	int i;
	sem_t *sem;

	for (i = 0; i < SEM_NSEMS_MAX; i++) {
		sem = &(_named_semaphores[i]);

		if ((sem->magic != SEM_MAGIC) || (sem->name == NULL))
		    continue;

		/*
		 * If absolute, need to close properly as other processes
		 * might be waiting.
		 * If relative, unlink it.
		 */
		if (sem->name[0] == '/')
			(void) sem_close(sem);
		else
			(void) unlink(sem->name);
		sem->magic = 0;		/* Doesn't exist anymore */
	}
}


/*
 * Close a named semaphore.
 */
int
sem_close(sem)
	sem_t *sem;
{
	int status = 0;
	int value;

	if ((sem == NULL) || (sem->magic != SEM_MAGIC) || (sem->name == NULL)) {
		errno = EINVAL;
		return(-1);
	}

	if (_lock_semaphore(sem) != 0)
		return(-1);

	if (--sem->refct == 0) {
		/* If locks are held on a named semaphore, release them */
		if (sem->name && sem->lockct > 0) {
			if (_get_semaphore(sem, &value))
				goto fail;
			value -= sem->lockct;
			if (_store_semaphore(sem, value))
				goto fail;
		}

		/* Mark as dead by clearing magic */
		sem->magic = 0;
		_unlock_semaphore(sem);
		(void) close(sem->fd);

		/* Free name */
		(void) free(sem->name);
		sem->name = NULL;
	} else
		_unlock_semaphore(sem);

	return(0);

fail:
	status = errno;
	_unlock_semaphore(sem);
	errno = status;
	return(-1);
}


/*
 * Destroy an unnamed semaphore.
 * Don't allow this if any threads are blocked.
 */
int
sem_destroy(sem)
	sem_t *sem;
{
	int value;
	int status = 0;

	if ((sem == NULL) || (sem->magic != SEM_MAGIC) || (sem->name != NULL)) {
		errno = EINVAL;
		return(-1);
	}

	if (_lock_semaphore(sem) != 0)
		return(-1);

	if (_get_semaphore(sem, &value))
		goto fail;

	if (value <= 0) {
		_unlock_semaphore(sem);
		errno = EBUSY;
		return(-1);
	}

	/* Mark as dead by clearing magic */
	sem->magic = 0;
	_unlock_semaphore(sem);

	return(0);

fail:
	status = errno;
	_unlock_semaphore(sem);
	errno = status;
	return(-1);
}


/*
 * Get the value of a semaphore
 */
int
sem_getvalue(sem, sval)
	sem_t *sem;
	int *sval;
{
	int value;
	int status;

	if ((sem == NULL) || (sem->magic != SEM_MAGIC)) {
		errno = EINVAL;
		return(-1);
	}

	if (_lock_semaphore(sem) != 0)
		return(-1);

	if (_get_semaphore(sem, sval))
		goto fail;

	_unlock_semaphore(sem);

	return(0);

fail:
	status = errno;
	_unlock_semaphore(sem);
	errno = status;
	return(-1);
}


/*
 * Create an unnamed semaphore.
 */
int
sem_init(sem, pshared, value)
	sem_t *sem;
	int pshared;
	unsigned value;
{
	int status;

	/* No storage, or already exists? */
	if ((sem == NULL) || (sem->magic == SEM_MAGIC)) {
		errno = EINVAL;
		return(-1);
	}

	/*
	 * We specifically do not support this.  If processes need to share
	 * a semaphore, they should use an absolute named semaphore.
	 */
	if (pshared != 0) {
		errno = EOPNOTSUPP;
		return(-1);
	}

	/*
	 * A value of 0 or less is accepted as a process could do this to
	 * skip the initial locking call, although it may be a bit sloppy.
	 */
	if (value > SEM_VALUE_MAX) {
		errno = EINVAL;
		return(-1);
	}

	/* Also sets sem->name == NULL for unnamed */
	bzero(sem, sizeof(sem_t));

	sem->value = value;

	/* Now mark the semaphore as existing */
	sem->magic = SEM_MAGIC;

	return(0);
}


/*
 * Open a named semaphore.
 */
sem_t *
#if __STDC__
sem_open(const char *name, int oflag, ...)
#else
sem_open(name, oflag, va_alist)
	const char *name;
	int oflag;
	va_dcl;
#endif
{
	va_list ap;
	mode_t mode;
	unsigned value;
	char *fullname = NULL;
	int fd = -1;
	int status, i, are_creating = FALSE;
	sem_t *sem = NULL;
	struct stat sb;

	/* Force O_RDWR if isn't already */
	oflag |= O_RDWR;

	/* Only accept O_CREAT, O_EXCL, O_RDWR */
	if (oflag & ~(O_CREAT | O_EXCL | O_RDWR)) {
		errno = EINVAL;
		return(SEM_FAILED);
	}

	fullname = _convert_name(name);
	if (fullname == NULL)
		return(SEM_FAILED);

	if (oflag & O_CREAT) {
#if __STDC__
		va_start(ap, oflag);
#else
		va_start(ap);
#endif
		mode = va_arg(ap, unsigned);
		value = va_arg(ap, unsigned);
		va_end(ap);

		/*
		 * A value of 0 or less is accepted as a process could do
		 * this to skip the initial locking call, although it may
		 * be a bit sloppy.
		 */
		if (value > SEM_VALUE_MAX) {
			errno = EINVAL;
			return(SEM_FAILED);
		}
	}

	/*
	 * See if the semaphore is already open as we MUST return same
	 * sem_t address and not just another reference to same file...
	 * We do a stat as we need file name and inode number to be sure
	 * it is the same file.  If this process is already holding the
	 * file open, we can guarantee that the underlying inode cannot
	 * be reused when a different process unlinks that file and
	 * creates a new one of the same name.  So the inode is a
	 * sufficient test of uniqueness.
	 */
	status = stat(fullname, &sb);
	if (status == 0) {			/* File exists */
		/* Wasn't supposed to? */
		if ((oflag & O_CREAT) && (oflag & O_EXCL)) {
			errno = EEXIST;
			goto fail;
		}
	} else {				/* Some error */
		if (errno != ENOENT)		/* Other than no file? */
			goto fail;

		if ((oflag & O_CREAT) == 0)	/* No file and not creating? */
			goto fail;

		/* No file, but we are creating, need to store initial value */
		are_creating = TRUE;
	}

	/* Lock the array */
	if (THREAD_SAFE())
		if (pthread_mutex_lock(&_named_mutex))
			goto fail;

	/* Walk the array */
	if (are_creating == FALSE)
		for (i = 0; i < SEM_NSEMS_MAX; i++) {
			sem = &(_named_semaphores[i]);

			/* Look only for named semaphores */
			if ((sem->magic != SEM_MAGIC) || (sem->name == NULL))
				continue;

			/* Same inode, same name and same time? */
			if ((sem->inode == sb.st_ino) &&
			    (strcmp(fullname, sem->name) == 0)) {
				/* Extra open done on this mutex */
				if (_lock_semaphore(sem) != 0)
					goto fail;

				sem->refct++;

				_unlock_semaphore(sem);

				/* Return the entry */
				goto ok;
			}
		}

	/* Array remains locked */

	/* Get a new entry */
	for (i = 0; i < SEM_NSEMS_MAX; i++) {
		sem = &(_named_semaphores[i]);

		if (sem->magic == SEM_MAGIC)
		    continue;

		/* Set an initial value if file didn't exist */
		if (are_creating == TRUE) {
			if ((fd = open(fullname, oflag, mode)) < 0)
				goto fail;
			if (write(fd, &value, sizeof(int)) != sizeof(int))
				goto fail;
			(void) close(fd);
			fd = -1;

			/* Get the file information we created */
			if (stat(fullname, &sb) != 0)
				goto fail;
		}

		/* Fill in the data */
		sem->name = fullname;
		sem->refct = 1;
		sem->inode = sb.st_ino;

		/* Now mark as existing */
		sem->magic = SEM_MAGIC;

		goto ok;
	}

	/* If we get here, there is no room for a new semaphore */

	errno = ENOSPC;
	/* Fall through to fail: */

fail:
	status = errno;
	if (THREAD_SAFE())
		(void) pthread_mutex_unlock(&_named_mutex);

	if (fd != -1)
		(void) close(fd);

	if (fullname != NULL)
		(void) free(fullname);

	errno = status;
	return(SEM_FAILED);

ok:
	/* Before unlocking, set up atexit call */
	if (atexit_setup == FALSE) {
		atexit(_sem_cleanup);
		atexit_setup = TRUE;
	}

	if (THREAD_SAFE())
		(void) pthread_mutex_unlock(&_named_mutex);
	return(sem);
}


/*
 * Unlock (increment) the given semaphore.
 */
int
sem_post(sem)
	sem_t *sem;
{
	int value;
	int status;

	if ((sem == NULL) || (sem->magic != SEM_MAGIC)) {
		errno = EINVAL;
		return(-1);
	}

	if (_lock_semaphore(sem) != 0)
		return(-1);

	if (_get_semaphore(sem, &value))
		goto fail;

	value++;
	if (sem->name)			/* Gave up a lock on named semaphore */
		sem->lockct--;
	if (_store_semaphore(sem, value))
		goto fail;

	_unlock_semaphore(sem);

	return(0);

fail:
	status = errno;
	_unlock_semaphore(sem);
	errno = status;
	return(-1);
}


/*
 * Try to lock a semaphore.  Specify an absolute time up to which we can
 * get the semaphore before giving up.  If the timeout is NULL, will
 * wait forever.
 */
int
sem_timedwait(sem, abs_timeout)
	sem_t *sem;
	const struct timespec *abs_timeout;
{
	int ret;
	struct timespec now;

	while (1) {
		ret = sem_trywait(sem);
		if ((ret == 0) || ((ret == -1) && (errno != EAGAIN)))
			return(ret);

		/* Have we gone over the time limit? */
		if (abs_timeout) {
			clock_gettime(CLOCK_REALTIME, &now);
			if (timespeccmp(&now, abs_timeout, >=)) {
				errno = ETIMEDOUT;
				return(-1);
			}
		}

		/* Sleep a little and try again */
		usleep(1000);
	}

	/* Not reached */
}


/*
 * [Should really be called sem_trylock()].  Try to lock (decrement) the
 * given semaphore.  Do not wait if it cannot be locked immediately.
 */
int
sem_trywait(sem)
	sem_t *sem;
{
	int value, ret, status;

	if ((sem == NULL) || (sem->magic != SEM_MAGIC)) {
		errno = EINVAL;
		return(-1);
	}

	if (_lock_semaphore(sem) != 0)
		return(-1);

	if (_get_semaphore(sem, &value))
		goto fail;
	if (value <= 0) {
		status = EAGAIN;
		ret = -1;
	} else if (value > 0) {
		value--;
		if (sem->name)		/* Got a lock on named semaphore */
			sem->lockct++;
		if (_store_semaphore(sem, value))
			goto fail;
		status = 0;
		ret = 0;
	}

	_unlock_semaphore(sem);
	errno = status;
	return(ret);

fail:
	status = errno;
	_unlock_semaphore(sem);
	errno = status;
	return(-1);
}


/*
 * Remove named semaphore.
 * We don't do special checking on the name, but use the same construction
 * rules for sem_open().
 */
int
sem_unlink(const char *name)
{
	char *fullname = NULL;

	fullname = _convert_name(name);
	if (fullname == NULL)
		return(-1);
	
	return(unlink(fullname));
}


/*
 * Try to lock a semaphore, wait indefinitely if needed.
 */
int
sem_wait(sem)
	sem_t *sem;
{
	return(sem_timedwait(sem, NULL));
}
