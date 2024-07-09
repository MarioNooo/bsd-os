#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <limits.h>
#include <pthread.h>

/*
 * sem_open is supposed to return the same address for every call to open a
 * named semaphore that still exists, hence refct and inode.
 */
typedef struct {
	uint		magic;	/* Magic value */
	char		*name;	/* Name of this semaphore, NULL if unnamed */

	/* For named semaphores */
	uint		refct;	/* Number of opens on this sem_t */
	uint		lockct;	/* Number of locks we hold on this sem_t */
	int		inode;	/* Underlying inode, in case sem_unlink used */
	int		fd;	/* When accessing the file */

	/* For unnamed semaphores */
	int		value;

	/* For locking as signal handler could call sem_post */
	sigset_t	original_mask;
} sem_t;

#define SEM_MAGIC	0x00c0ffee
#define SEM_VALUE_MAX	INT_MAX
#define SEM_FAILED	NULL
#define SEM_NSEMS_MAX	1024
#if (SEM_NSEMS_MAX < OPEN_MAX)
#error SEM_NSEMS_MAX < OPEN_MAX
#endif

int	sem_close __P((sem_t *));
int	sem_destroy __P((sem_t *));
int	sem_getvalue __P((sem_t *, int *));
int	sem_init __P((sem_t *, int, unsigned));
sem_t	*sem_open __P((const char *, int, ...));
int	sem_post __P((sem_t *));
int	sem_timedwait __P((sem_t *, const struct timespec *));
int	sem_trywait __P((sem_t *));
int	sem_unlink __P((const char *));
int	sem_wait __P((sem_t *));

#endif	/* SEMAPHORE_H */
