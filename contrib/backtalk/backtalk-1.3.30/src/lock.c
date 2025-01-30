/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* This file (and it's header file) define functions and macros to implement
 * locking on various platforms.
 *
 * The following can be called immediately after opening a file.  fd is the
 * file descriptor returned from open(), and fn is the filename.
 *
 *     lock_exclusive(fd,fn) - obtain exclusive lock on file.
 *     lock_shared(fd,fn) - obtain shared lock on file.
 *
 * For the LOCK_FCNTL case, it is necessary that write access exist for files
 * that we are requesting exclusive locks on, and read access exists for files
 * we are requesting shared locks on.
 *
 * Unlocking can happen before or after closing the file:
 *
 *	unlock(fn) - discard a lock on a file we have closed.
 *	unlock_active(fn) - discard a lock on a file we still want open.
 *
 * These work for systems with flock(), lockf(), fcntl() or neither.  fcntl()
 * appears to be the best since it works over shared file systems.  flock()
 * may not, but does do real shared locks.  lockf() doesn't implement shared
 * locks, so lock_shared() will actually give an exclusive lock.  If none of
 * these available, we use a lock file.  This is clumsy, slow, and should
 * probably be avoided.  All locks are exclusive with lock files.
 *
 * If we are to share files with Yapp or Picospan, it is important to use a
 * compatible locking scheme.  Dave Thaler's Yapp source doesn't support
 * fcntl() locks, but fcntl() locks interact with lockf() locks, so that's
 * alright.  However flock() locks are independent from lockf() locks (on
 * some systems, at least), and, of course, lockfiles are independent from
 * everything.
 *
 * The code for the lockfiles is written using Dave Thaler's Yapp 2.3 source
 * as a reference to ensure compatibility.
 */

#include "backtalk.h"

#if !defined(LOCK_FCNTL) && !defined(LOCK_FLOCK) && !defined(LOCK_LOCKF) && !defined(LOCK_FILE)
#define LOCK_FCNTL
#endif

#ifdef LOCK_FCNTL

#include "lock.h"

void setlock(int fd, int type)
{
    struct flock lk;

    lk.l_type= type;
    lk.l_whence= 0;
    lk.l_start= 0L;
    lk.l_len= 0L;
    fcntl(fd,F_SETLKW,&lk);
}

#elif defined(LOCK_FILE)

/* Lock Files */

#include <errno.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include "lock.h"

#ifdef LOCK_DEBUG
void r_lock_exclusive(int fd, char *fn)
#else
void lock_exclusive(int fd, char *fn)
#endif
{
    char bf[BFSZ];
    int lfd;
    int timeout= 0;

    sprintf(bf,"%s.lock",fn);
    while ((lfd= open(bf,O_WRONLY|O_CREAT|O_EXCL,0400)) < 0 &&
	    errno==EEXIST && timeout++ < 10)
	sleep(1);
    if (lfd >= 0) close(lfd);
    /* If we fail to create the lock file -- go ahead anyway */
}


#ifdef LOCK_DEBUG
void r_unlock(char *fn)
#else
void unlock(char *fn)
#endif
{
    char bf[BFSZ];

    sprintf(bf,"%s.lock",fn);
    unlink(bf);
}
#else
#include "lock.h"
#endif /* LOCK_FILE */

#ifdef LOCK_DEBUG

void lock_exclusive(int fd, char *fn)
{
    fprintf(stderr,"Exclusive Lock on %s (%d)\n",fn,fd);
    fflush(stderr);
    r_lock_exclusive(fd,fn);
}

void lock_shared(int fd, char *fn)
{
    fprintf(stderr,"Shared Lock on %s (%d)\n",fn,fd);
    fflush(stderr);
    r_lock_shared(fd,fn);
}

void lock(int ex, int fd, char *fn)
{
    fprintf(stderr,"%s Lock on %s (%d)\n",ex?"EXCL":"SHAR",fn,fd);
    fflush(stderr);
    r_lock(ex,fd,fn);
}

void unlock_active(int fd, char *fn)
{
    fprintf(stderr,"Unlock (active) on %s (%d)\n",fn,fd);
    fflush(stderr);
    r_unlock_active(fd,fn);
}

void unlock(char *fn)
{
    fprintf(stderr,"Unlock on %s\n",fn);
    fflush(stderr);
    r_unlock(fd);
}

#endif /* LOCK_DEBUG */
