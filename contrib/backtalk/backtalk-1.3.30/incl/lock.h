/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#ifndef LOCK_DEBUG
#if defined(LOCK_FLOCK)				/* lock with flock() call */

#include <sys/file.h>
#define lock_exclusive(fd,fn) flock(fd,LOCK_EX)
#define lock_shared(fd,fn) flock(fd,LOCK_SH)
#define lock(ex,fd,fn) flock(fd,ex?LOCK_EX:LOCK_SH)
#define unlock_active(fd,fn) flock(fd,LOCK_UN)
#define unlock(fn)

#elif defined(LOCK_LOCKF)			/* lock with lockf() call */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#define lock_exclusive(fd,fn) lockf(fd,F_LOCK,0L);
#define lock_shared(fd,fn) lockf(fd,F_LOCK,0L);
#define lock(ex,fd,fn) lockf(fd,F_LOCK,0L);
#define unlock_active(fd,fn) lockf(fd,F_ULOCK,0L);
#define unlock(fn)

#elif defined(LOCK_FILE)			/* lock with lock files */

#include <fcntl.h>
#define lock_shared(fd,fn) lock_exclusive(fd,fn)
#define lock(ex,fd,fn) lock_exclusive(fd,fn)
#define unlock_active(fd,fn) unlock(fn)
void lock_exclusive(int fd, char *fn);
void unlock(char *fn);

#else 						/* lock with fnctl() call */
#ifndef LOCK_FCNTL
#define LOCK_FCNTL
#endif

#include <fcntl.h>
void setlock(int fd, int type);
#define lock_exclusive(fd,fn) setlock(fd,F_WRLCK);
#define lock_shared(fd,fn) setlock(fd,F_RDLCK);
#define lock(ex,fd,fn) setlock(fd,ex?F_WRLCK:F_RDLCK);
#define unlock_active(fd,fn) setlock(fd,F_UNLCK);
#define unlock(fn)

#endif

#else

/* =========================== Version for Debugging ======================= */

void lock_exclusive(int fd, char *fn);
void unlock(char *fn);
void lock_shared(int fd, char *fn);
void lock(int ex, int fd, char *fn);
void unlock_active(int fd, char *fn);

#if defined(LOCK_FLOCK)				/* lock with flock() call */

#include <sys/file.h>
#define r_lock_exclusive(fd,fn) flock(fd,LOCK_EX)
#define r_lock_shared(fd,fn) flock(fd,LOCK_SH)
#define r_lock(ex,fd,fn) flock(fd,ex?LOCK_EX:LOCK_SH)
#define r_unlock_active(fd,fn) flock(fd,LOCK_UN)
#define r_unlock(fn)

#elif defined(LOCK_LOCKF)			/* lock with lockf() call */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#define r_lock_exclusive(fd,fn) lockf(fd,F_LOCK,0L);
#define r_lock_shared(fd,fn) lockf(fd,F_LOCK,0L);
#define r_lock(ex,fd,fn) lockf(fd,F_LOCK,0L);
#define r_unlock_active(fd,fn) lockf(fd,F_ULOCK,0L);
#define r_unlock(fn)

#elif defined(LOCK_FILE)			/* lock with lock files */

#include <fcntl.h>
#define r_lock_shared(fd,fn) lock_exclusive(fd,fn)
#define r_lock(ex,fd,fn) lock_exclusive(fd,fn)
#define r_unlock_active(fd,fn) unlock(fn)

#else						/* lock with fnctl() call */
#ifndef LOCK_FCNTL
#define LOCK_FCNTL
#endif

#include <fcntl.h>
#define r_lock_exclusive(fd,fn) setlock(fd,F_WRLCK);
#define r_lock_shared(fd,fn) setlock(fd,F_RDLCK);
#define r_lock(ex,fd,fn) setlock(fd,ex?F_WRLCK:F_RDLCK);
#define r_unlock_active(fd,fn) setlock(fd,F_UNLCK);
#define r_unlock(fn)

#endif

#endif
