/*
 *  Start everything up.
 */

#include <sys/types.h>

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

#include "libgrex.h"

void
threadspawn(Thread *context, void *(*func)(void *), void *arg)
{

	if (pthread_create((pthread_t *)context, NULL, func, arg)) {
		fatal("pthread_create");
	}
}

void
wrlock(RWLock *lock)
{

	if (pthread_rwlock_wrlock((pthread_rwlock_t *)lock) != 0) {
		fatal("pthread_rwlock_wrlock");
	}
}

void
rdlock(RWLock *lock)
{

	if (pthread_rwlock_rdlock((pthread_rwlock_t *)lock) != 0) {
		fatal("pthread_rwlock_unlock");
	}
}

void
unlock(RWLock *lock)
{

	if (pthread_rwlock_unlock((pthread_rwlock_t *)lock) != 0) {
		fatal("pthread_rwlock_unlock");
	}
}
