/*
 * Maximum number of simultaneously logged in users.  If this is exceeded,
 * robocop will stop killing orphaned processes and will add up memory usage
 * for only as many users as fit in the table.
 */
#define MAXUSERS  256

/*
 * Number of different tty's or psuedotty's that users could log into.  If
 * this number is exceeded we'll slow down a tiny bit, but otherwise we will
 * be OK.
 */
#define MAXTTYS  256

/*
 * Maximum number of processes to protect.  Only things like sendmail that we
 * don't want to kill and their child processes count.  If this is exceeded,
 * then we may fail to protect some child processes.
 */
#define MAXPROCS 256

/*
 * Maximum number of users that will simultaneously be running 'screen' in
 * foreground.  If this is exceeded killing of orphaned screen processes
 * stops until the users drop below this level again.
 */
#define MAXSCREENERS 32

/*
 * Maximum command length, with arguments.  Longer commands are truncated.
 * This is used for logging only.
 */
#define MAXCALEN 100

/* Units for defines */
#define MINUTES 60
#define HOURS 3600
#define MEGABYTES (1024*1024)

/* How long to sleep between scans */
#define INTERVAL (8*MINUTES)

/* Kill ftpds that are older than this */
#define FTP_TOO_OLD (6*HOURS)

/* Kill fingerd's that are older than this */
#define FINGERD_TOO_OLD (10*MINUTES)

/* If a user is using more memory or processes than this, log him */
#define LOG_MEMORY (16*MEGABYTES)
#define LOG_NPROC  16

/* If a user is using more memory or processes than this, kill him.  */
#define MAX_MEMORY (40*MEGABYTES)	/* Must be >= LOG_MEMORY */
#define MAX_NPROC  31		/* Must be >= LOG_NPROC */

/* Staff processes don't count toward toward memory limits */
#define STAFF_GID 20

/*
 * When doing a killu, this is the maximum number of passes we do before
 * giving up.
 */
#define MAX_KILL_PASS 45
#define MIN_KILL_UID 131072

/*
 * These defines decide if we want to protect various processes that we might
 * not want to kill, but which can be mistaken for orphans. Not protecting
 * sshd kills only pty-less connections, not normal ones.
 */
/* #define PROTECT_SSH_AGENT     /* No - broken on Grex - never exits */
/* #define PROTECT_RDIST         /* No - Grex doesn't run it at all */
#define PROTECT_SSHD		/* Yes */
#define PROTECT_SCREEN		/* Yes */
#define PROTECT_SENDMAIL	/* Yes - Though we really use postfix */
#define PROTECT_POSTFIX		/* Yes */
#define PROTECT_DOVECOT		/* Yes */
#define PROTECT_IRCD		/* Yes */

/*
 * On old Grex named and inetd had the habit of dieing.  Robocop was taught
 * to monitor them and restart them if they died.  THIS CODE HAS NOT BEEN
 * UPDATED FOR FOR OPENBSD.
 */
/* #define RESTART_NAMED	/* NO - HAS NOT BEEN PORTED TO OPENBSD */
/* #define RESTART_INETD	/* NO - HAS NOT BEEN PORTED TO OPENBSD */

/* Paths of system commands */
#ifndef PATH_SH
#define PATH_SH "/bin/sh"
#endif
#ifndef PATH_SENDMAIL
/* The real sendmail program - not used - it'd be cool to read mailer.conf */
/* #define PATH_SENDMAIL "/usr/libexec/sendmail/sendmail" */
/* Postfix is our MTA */
#define PATH_POSTFIX "/usr/local/libexec/postfix/master"
/* The sendmail wrapper program, which we also protect */
#define PATH_SENDMAIL "/usr/sbin/sendmail"
#endif
#ifndef PATH_SCREEN
#define PATH_SCREEN "/usr/local/bin/screen"
#endif
#ifndef PATH_TMUX
#define PATH_TMUX "/usr/bin/tmux"
#endif
#ifndef PATH_FTPD
#define PATH_FTPD "/usr/libexec/ftpd"
#endif
#ifndef PATH_SSHD
#define PATH_SSHD "/usr/sbin/sshd"
#endif
#ifndef PATH_SSH_AGENT
#define PATH_SSH_AGENT "/usr/bin/ssh-agent"
#endif
#ifndef PATH_RDIST
#define PATH_RDIST "/usr/bin/rdist"
#endif
#ifndef PATH_INETD
#define PATH_INETD "/usr/sbin/inetd"
#endif
#ifndef PATH_NAMED
#define PATH_NAMED "/usr/sbin/named"
#endif
#ifndef PATH_IRCD
#define PATH_IRCD "/usr/local/bin/ircd"
#endif
#ifndef PATH_DOVECOT
#define PATH_DOVECOT "/usr/local/sbin/dovecot"
#endif

#ifndef UTMP
#define UTMP "/var/run/utmp"
#endif

#ifndef LOGFILE
#define LOGFILE "/var/log/robocop.log"
#endif

/* Signals to use to kill processes */
#ifdef DEBUG
#define SOFTKILL 0
#define HARDKILL 0
#else
#define SOFTKILL SIGTERM
#define HARDKILL SIGKILL
#endif

/* Signal to use to awaken robocop */
#define SIGWAKE SIGHUP
