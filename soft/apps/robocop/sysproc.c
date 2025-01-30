/************************* RECOGNIZE SYSTEM PROCS **************************
 * For each system process that we like to be able to recognize, we keep the
 * device and inode number of the real binary.  It is possible that there
 * could be more than one legal dev/inode since a binary could change during
 * the run of backtalk, and both old and new versions might be running
 * simultaneously.  If a process claims to be sendmail (for example), it
 * needs to have matching fingerprint information.  On old Grex there was
 * no easy way to get the device/inode from which a process was loaded, so
 * we used the a.out headers as the fingerprint instead.
 */

#include "config.h"
#include "extern.h"

#include <kvm.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>

/* These globals will have indexes into the sysproc table stored into them */

int             sh_id, ftpd_id;
#ifdef PROTECT_SENDMAIL
int             sendmail_id;
#endif
#ifdef PROTECT_POSTFIX
int             postfix_id;
#endif
#ifdef PROTECT_SCREEN
int             screen_id;
#endif
#ifdef PROTECT_RDIST
int             rdist_id;
#endif
#ifdef PROTECT_SSHD
int             sshd_id;
#endif
#ifdef PROTECT_SSH_AGENT
int             ssh_agent_id;
#endif
#ifdef PROTECT_IRCD
int		ircd_id;
#endif
#ifdef PROTECT_DOVECOT
int		dovecot_id;
#endif

/* These are the linked list nodes of process fingerprint data */

struct fingerprint {
	dev_t           device;
	ino_t           inode;
	struct fingerprint *next;
};

/*
 * This gives names and path names.  Note each can have multiple fingerprints
 * associated with it, if the binary file changes during robocop's lifetime.
 * All versions will be accepted for as long as robocop lives.
 */

struct sysproc {
	int            *var;
	char           *name;
	int             partlen;/* If process name can be longer, this min
				 * length */
	char           *path;
	struct fingerprint *fp;
}               sysprocs[] = {
	{
		&sh_id, "sh", 0, PATH_SH, NULL
	},
#ifdef PROTECT_SENDMAIL
	{
		&sendmail_id, "sendmail", 0, PATH_SENDMAIL, NULL
	},
#endif
#ifdef PROTECT_POSTFIX
	{
		&postfix_id, "master", 0, PATH_POSTFIX, NULL
	},
#endif
	{
		&ftpd_id, "ftpd", 0, PATH_FTPD, NULL
	},
#ifdef PROTECT_SCREEN
	{
		&screen_id, "screen", 0, PATH_SCREEN, NULL
	},
#endif
#ifdef PROTECT_RDIST
	{
		&rdist_id, "rdist", 0, PATH_RDIST, NULL
	},
#endif
#ifdef PROTECT_SSHD
	{
		&sshd_id, "sshd", 0, PATH_SSHD, NULL
	},
#endif
#ifdef PROTECT_SSH_AGENT
	{
		&ssh_agent_id, "ssh-agent", 0, PATH_SSH_AGENT, NULL
	},
#endif
#ifdef PROTECT_IRCD
	{
		&ircd_id, "ircd", 0, PATH_IRCD, NULL
	},
#endif
#ifdef PROTECT_DOVECOT
	{
		&dovecot_id, "dovecot", 0, PATH_DOVECOT, NULL
	},
#endif
	{
		NULL, NULL, 0, NULL, NULL
	}
};


/*
 * INIT_SYSPROC - Get initial fingerprints for everything in the sysprocs
 * table.  Under SunOS the fingerprints were the a.out headers of the files,
 * but OpenBSD lets us get the dev/inode values of the file, which is much
 * more satisfactory.  We refuse to run if this doesn't work.
 */

void 
init_sysproc()
{
	int             i;
	struct stat     st;

	for (i = 0; sysprocs[i].var != NULL; i++) {
		*sysprocs[i].var = i;
		if (stat(sysprocs[i].path, &st)) {
			fprintf(stderr, "Cannot stat program file %s header\n",
				sysprocs[i].path);
			exit(1);
		}
		sysprocs[i].fp = (struct fingerprint *)
			malloc(sizeof(struct fingerprint));
		if (sysprocs[i].fp == NULL) {
			fprintf(stderr, "Out of Memory\n");
			exit(1);
		}
		sysprocs[i].fp->device = st.st_dev;
		sysprocs[i].fp->inode = st.st_ino;
		sysprocs[i].fp->next = NULL;
#ifdef VERBOSE
		if (logfp)
			fprintf(logfp, "SYSPROC %s DEV=%s INODE=%d\n",
				sysprocs[i].name, devname(st.st_dev, S_IFBLK), st.st_ino);
#endif
	}
}


/*
 * IS_SYSPROC - Given a kinfo_proc pointer for a process, decide if it is the
 * system process with the given index.  Return true if it is.  Since this
 * can be called several times on the same process, and digging out device
 * and inode numbers is moderately expensive, we cache the most recent
 * values.  We don't flush the cache between scans, but that should be OK.
 */

int 
is_sysproc(int i, kvm_t * kd, struct kinfo_proc * pr)
{
	struct vnode    vnode;
	struct inode    inode;
	static pid_t    saved_pid = 0;
	static dev_t    saved_device;
	static ino_t    saved_inode;

	/* First check the name.  If the name doesn't match, reject it */
	if (sysprocs[i].partlen > 0) {
		if (strncmp(sysprocs[i].name, pr->p_comm, sysprocs[i].partlen))
			return 0;
	} else {
		if (strcmp(sysprocs[i].name, pr->p_comm))
			return 0;
	}

	if (saved_pid != pr->p_pid) {
		/* Load the proc structure for this program */
		struct proc proc;

		if (kvm_read(kd, (u_long) pr->p_paddr,
		    &proc, sizeof(proc)) != sizeof(proc)) {
			if (logfp)
				fprintf(logfp,
				"Failed to read vnode for process %d - %s\n",
					proc.p_pid, kvm_geterr(kd));
			return 0;
		}

		/* Load the vnode structure for the program text */
		if (proc.p_textvp == NULL)
			return 0;

		if (kvm_read(kd, (u_long) proc.p_textvp,
		    &vnode, sizeof(struct vnode)) != sizeof(struct vnode)) {
			if (logfp)
				fprintf(logfp,
				"Failed to read vnode for process %d - %s\n",
					proc.p_pid, kvm_geterr(kd));
			return 0;
		}
		/* Check that this is UFS or EXT2FS.  Only UFS should happen */
		if (vnode.v_tag != VT_UFS && vnode.v_tag != VT_EXT2FS)
			return 0;

		/* Load the inode structure for the program text */
		if (kvm_read(kd, (u_long) vnode.v_data,
		    &inode, sizeof(struct inode)) != sizeof(struct inode)) {
			if (logfp)
				fprintf(logfp,
				"Failed to read inode for process %d - %s\n",
					proc.p_pid, kvm_geterr(kd));
			return 0;
		}
		saved_device = inode.i_dev;
		saved_inode = inode.i_number;
	}
	/* Finally, check if it is the right device/inode */
	return check_sysproc(i, saved_device, saved_inode);
}


/*
 * CHECK_SYSPROC - Given an index into the sysproc table, a device and an
 * inode, see if they match.  If not, re-stat the file to double check that
 * this is not a new version.  Returns 1 if it seems to be the real thing.
 */

int 
check_sysproc(int i, dev_t device, ino_t inode)
{
	struct fingerprint *f;
	struct stat     st;

	for (f = sysprocs[i].fp; f != NULL; f = f->next)
		if (f->device == device && f->inode == inode)
			return 1;

	/*
	 * Doesn't match any stored fingerprint - Check to make sure
	 * installed binary hasn't changed
	 */
	if (stat(sysprocs[i].path, &st)) {
		if (logfp)
			fprintf(logfp, "Can't stat program file %s\n");
		/* Not sure it's a fake, so pass it */
		return 1;
	}
	if (st.st_dev != device || st.st_ino != inode) {
		if (logfp)
			fprintf(logfp,
				"Fraudulent %s imitation found (device=%s, inode=%d)\n",
			 sysprocs[i].path, devname(device, S_IFBLK), inode);
		return 0;
	}
	/* Got a new version - add it to the list */
	if (logfp)
		fprintf(logfp, "New version of %s detected (device=%s, inode=%d)\n",
		  sysprocs[i].path, devname(st.st_dev, S_IFBLK), st.st_ino);

	f = (struct fingerprint *) malloc(sizeof(struct fingerprint));
	if (f == NULL) {
		if (logfp)
			fprintf(logfp, "Out of memory - forgetting new fingerprint\n");
		return 1;
	}
	f->device = st.st_dev;
	f->inode = st.st_ino;
	f->next = sysprocs[i].fp;
	sysprocs[i].fp = f;

	return 1;
}
