/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Routines to maintain a session database in a file */

#include "backtalk.h"
#include "sysdict.h"
#include "entropy.h"

#ifdef ID_SESSION	/* This is all used only with cookie authentication */
char *showopt_session_module= "sess_file (file="SESSION_FILE")";

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
extern char *crypt();

#include "sess.h"
#include "lock.h"
#include "udb.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


struct sess_entry {
    char id[SESSION_ID_LEN];	/* Session id string */
    char user[MAX_LOGIN_LEN];	/* User name */
    time_t time;		/* Last access time */
    unsigned long ip;		/* User's IP number */
    };


int session_fd= -1;


/* OPEN_SESSION_FILE - Open the session database.  Bombs on failure.
 */

void open_session_file()
{
    if (session_fd >= 0)
    {
	lseek(session_fd, 0L, SEEK_SET);	/* rewind */
	return;
    }

    if ((session_fd= open(SESSION_FILE,O_RDWR|O_CREAT,0600)) < 0)
	die("Cannot open "SESSION_FILE);
}


/* CLOSE_SESSION_FILE - Close the session database.
 */

void close_session_file()
{
    if (session_fd >= 0)
	close(session_fd);
    session_fd= -1;
}


/* NEW_SESSION - Create new session for the given user.  Pass in a encrypted
 * password and the ip address (in ascii numbers-and-dots form) he connected
 * from.  You should already have checked that it is a valid login.  This will
 * create the session entry in the session file and return the session id
 * in malloced memory.  The caller should set the cookie.
 */

char *new_session(char *user, char *ip)
{
    char *sid;
    struct sess_entry sess, new_sess;
    time_t now= time(0L);
    off_t seek,offset;
    int retry= 0;

    /* Convert IP address to binary */
    if ((new_sess.ip= inet_addr(ip)) == -1)
	die("new_session: bad ip address %s",ip);

    /* Generate the new session id and make some a copy */
    sid= strdup(get_noise());

    /* Make the new session table entry */
    strncpy(new_sess.user, user, MAX_LOGIN_LEN);
    new_sess.time= now;

    open_session_file();
    lock_exclusive(session_fd,SESSION_FILE);

    /* Scan through file looking for an existing slot we can reuse.  We
     * always read the whole file, because we are also checking for
     * any duplicates of the session id we found.
     */

    offset= 0L;
    seek= -1L;
    while (read(session_fd, &sess, sizeof(struct sess_entry)) ==
	    sizeof(struct sess_entry))
    {
	/* If this is the first empty slot remember this position */
	if (seek < 0L && (sess.id[0] == '\0' ||
	    now - sess.time > idict(VAR_EXPIRE_SESSION)))
		seek= offset;

	/* In the rare case that we notice a duplicate of this
	 * session id already in the file, modify ours and restart
	 * the scan.
	 */
	if (!strncmp(sid,sess.id,SESSION_ID_LEN))
	{
	    if (++retry > 15) die("new_session: can't find new id %s",sid);
	    sid= strdup(get_noise());
	    offset= 0L;
	    seek= -1L;
	    lseek(session_fd, 0L, SEEK_SET);
	}
	else
	    offset+= sizeof(struct sess_entry);
    }

    /* If we found no empty slots, create a new one at the end */
    if (seek < 0L) seek= offset;

    /* Write out new session entry */
    lseek(session_fd, seek, SEEK_SET);
    strncpy(new_sess.id,sid,SESSION_ID_LEN);
    write(session_fd, &new_sess, sizeof(struct sess_entry));

    close_session_file();
    unlock(SESSION_FILE);
    return sid;
}


/* SESSION_LOOKUP - Look up the given session id in the session file.  If it
 * is there, and the process id (given here in ascii numbers-and-dots form)
 * matches, then update the last access time in the file and save the user
 * name in the "user" buffer (which should be at least MAX_LOGIN_LEN+1 bytes)
 * and return 0.  Otherwise return 1.
 *
 * Note that we do not lock the file while reading, not even a shared lock
 * (which may be implemented as an exclusive lock on some systems).  We don't
 * want this file to be a bottleneck on busy systems.  Thus there is some
 * risk that our slot will be swiped by the new_session() routine of some
 * other backtalk session between the time that we read it, and the time that
 * we update the last access time.  But since we carefully write out only
 * the new time, not the rest of the structure, we will only be extending
 * some other user's expiration time instead of our own.  This means that since
 * our own session slot is gone, we will get a timeout on the next access,
 * but we probably deserve it anyway, since our session must have expired on
 * on *this* access or else it wouldn't have been overwritten.
 */

int session_lookup(char *sid, char *ip, char *user)
{
    struct sess_entry sess;
    time_t now= time(0L);
    off_t offset= 0L;

#ifdef CHECK_SESSION_IP
    unsigned long bip;

    /* Convert ip address to binary */
    if ((bip= inet_addr(ip)) == -1) break;
#endif

    /* Open file */
    open_session_file();

    while (read(session_fd, &sess, sizeof(struct sess_entry)) ==
	sizeof(struct sess_entry))
    {
	offset+= sizeof(struct sess_entry);
	if (strncmp(sid,sess.id,SESSION_ID_LEN)) continue;

#ifdef CHECK_SESSION_IP
	if (bip != sess.ip) break;
#endif

	if (sess.time + idict(VAR_EXPIRE_SESSION) < now) break;

	/* Update the time field (only) */
	sess.time= now;
	lseek(session_fd, offset - sizeof(struct sess_entry) +
	    ((char *)(&sess.time)-(char *)(&sess.id)),
	    SEEK_SET);
	write(session_fd, &sess.time, sizeof(sess.time));

	strncpy(user,sess.user,MAX_LOGIN_LEN);
	user[MAX_LOGIN_LEN]= '\0';
	close_session_file();
	return 0;
    }
    close_session_file();
    return 1;
}


/* SESSION_DELETE - Delete the given session ID from the session file, if
 * it exists.  It is not an error if it does not exist.
 */

void session_delete(char *sid)
{
    struct sess_entry sess;
    off_t offset= 0L;

    /* Open file and skip header */
    open_session_file();
    lock_exclusive(session_fd,SESSION_FILE);

    while (read(session_fd, &sess, sizeof(struct sess_entry)) ==
	sizeof(struct sess_entry))
    {
	offset+= sizeof(struct sess_entry);
	if (!strncmp(sid,sess.id,SESSION_ID_LEN))
	{
	    /* Mark the session erased */
	    sess.id[0]= '\0';
	    lseek(session_fd, offset - sizeof(struct sess_entry), SEEK_SET);
	    write(session_fd, &sess, sizeof(struct sess_entry));
	    break;
	}
    }
    close_session_file();
    unlock(SESSION_FILE);
}

#endif /* ID_SESSION */
