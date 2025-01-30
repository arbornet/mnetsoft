/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Routines to read/write the Backtalk or Unix lastlog files.
 *
 * Backtalk lastlog is a binary file, an array of "time_t" values, indexed by
 * UID.  The unix one has some additional fields, which we merrily ignore here.
 */

#include "backtalk.h"

#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "last_log.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef LASTLOG_FILE

/* UPDATE_LASTLOG - update the lastlog file to indicate that the user with
 * the given uid has logged on.
 */

void update_lastlog(int uid)
{
    int fd;
    time_t now;

    /* Don't log anonymous users */
    if (uid < 0) return;

    if ((fd= open(LASTLOG_FILE,O_WRONLY|O_CREAT,0644)) < 0)
    	return;

    now= time(0L);
    lseek(fd,(long)(uid*sizeof(time_t)),0);
    write(fd,&now,sizeof(time_t));
    close(fd);
}


/* GET_LASTLOG (backtalk version) - get the last login time for the named
 * user.  Returns zero if we don't know.
 */

time_t get_lastlog(int uid)
{
    int fd;
    time_t when;

    /* Don't log anonymous users */
    if (uid < 0) return 0;

    if ((fd= open(LASTLOG_FILE,O_RDONLY)) < 0)
    	return 0;

    lseek(fd,(long)(uid*sizeof(time_t)),0);
    if (read(fd,&when,sizeof(time_t)) != sizeof(time_t))
    	when= 0;
    close(fd);

    return when;
}

#else
#ifdef UNIX_LASTLOG

#include <lastlog.h>
#ifdef PATHS_H
#include <paths.h>
#endif
#if !defined(LASTLOG) && defined(_PATH_LASTLOG)
#define LASTLOG _PATH_LASTLOG
#endif
#ifndef LASTLOG
#define LASTLOG "/usr/adm/lastlog"
#endif

/* GET_LASTLOG (unix version) - get the last login time for the named user.
 * Returns zero if we don't know.
 */

time_t get_lastlog(int uid)
{
    int fd;
    struct lastlog ll;

    /* Don't log anonymous users */
    if (uid < 0) return 0;

    if ((fd= open(LASTLOG,O_RDONLY)) < 0)
    	return 0;

    lseek(fd,(long)(uid*sizeof(struct lastlog)),0);
    if (read(fd,&ll,sizeof(struct lastlog)) != sizeof(struct lastlog))
    	ll.ll_time= 0;
    close(fd);

    return ll.ll_time;
}

#endif /*UNIX_LASTLONG*/
#endif /* LASTLOG_FILE */
