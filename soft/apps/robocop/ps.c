/*
 * This is a test program that basically does a 'ps'.  The idea is to figure
 * out how to dig all the information that robocop needs out of the OpenBSD
 * kernel.  This version is based on the kvm_getprocs() call. It would also
 * be possible to use the kvm_getproc2() call, or the /proc file system.  My
 * guess is that this is the faster alternative, and robocop does care about
 * speed.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>

#include <fcntl.h>
#include <kvm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
	PROCBUFSIZE = 16360,
	MAXCALEN = 256
};

struct keyword {
	int key;
	char *word;
};

static struct keyword s_vtype[] = {
	{VNON, "VNON"},
	{VREG, "VREG"},
	{VDIR, "VDIR"},
	{VBLK, "VBLK"},
	{VCHR, "VCHR"},
	{VLNK, "VLNK"},
	{VSOCK, "VSOCK"},
	{VFIFO, "VFIFO"},
	{VBAD, "VBAD"},
	{0, NULL}
};

static struct keyword s_vtagtype[] = {
	{VT_NON, "VT_NON"},
	{VT_UFS, "VT_UFS"},
	{VT_NFS, "VT_NFS"},
	{VT_MFS, "VT_MFS"},
	{VT_MSDOSFS, "VT_MSDOSFS"},
	/*{VT_LFS, "VT_LFS"},*/
	/*{VT_LOFS, "VT_LOFS"},*/
	/*{VT_FDESC, "VT_FDESC"},*/
	{VT_PORTAL, "VT_PORTAL"},
	/*{VT_NULL, "VT_NULL"},*/
	/*{VT_UMAP, "VT_UMAP"},*/
	/*{VT_KERNFS, "VT_KERNFS"},*/
	{VT_PROCFS, "VT_PROCFS"},
	{VT_AFS, "VT_AFS"},
	{VT_ISOFS, "VT_ISOFS"},
	/*{VT_UNION, "VT_UNION"},*/
	{VT_ADOSFS, "VT_ADOSFS"},
	{VT_EXT2FS, "VT_EXT2FS"},
	/*{VT_NCPFS, "VT_NCPFS"},*/
	{VT_VFS, "VT_VFS"},
	{VT_XFS, "VT_XFS"},
	{VT_NTFS, "VT_NTFS"},
	{0, NULL}
};

extern long pagesize, kperpage;

static char *
kw(struct keyword *kl, int n)
{
	int i;

	for (i = 0; kl[i].word != NULL; i++)
		if (kl[i].key == n)
			return kl[i].word;

	return "(bizzare)";
}

static void
yamlstr(char *buf, char *key, char *str, size_t buflen)
{
	strlcat(buf, key, buflen);
	strlcat(buf, ": ", buflen);
	strlcat(buf, str, buflen);
	if (str[strlen(str) - 1] != '\n')
		strlcat(buf, "\n", buflen);
}

static void
yamlint(char *buf, char *key, long value, size_t buflen)
{
	char val[64];
	snprintf(val, sizeof(val), "%ld", value);
	yamlstr(buf, key, val, buflen);
}

static void
strtoc(char *dst, char *src, size_t outlen)
{
	int ch;
	char *d, *s;

	if (outlen < 3)
		return;
	dst[0] = '\0';
	if (src == NULL)
		return;
	d = dst;
	s = src;
	outlen -= 3;	/* Save space for a nul. */
	*d++ = '"';
	outlen--;
	while ((ch = *s++) != '\0' && outlen > 0) {
		if (ch == '\n' || ch == '\r' || ch == '\0' || ch == '"') {
			if (outlen < 2)
				break;
			*d++ = '\\';
			switch(ch) {
			case '\n': ch = 'n'; break;
			case '\r': ch = 'r'; break;
			case '\0': ch = '0'; break;
			case '"': ch = '"'; break;
			}
		}
		*d++ = ch;
		outlen--;
	}
	*d++ = '"';
	*d = '\0';
}

static void
yamllst(char *buf, char *key, char **list, size_t buflen)
{
	char *prefix, singlearg[MAXCALEN + 1];

	if (list == NULL)
		return;
	strlcat(buf, key, buflen);
	prefix = ": [ ";
	while (*list) {
		strtoc(singlearg, *list, sizeof(singlearg));
		strlcat(buf, prefix, buflen);
		strlcat(buf, singlearg, buflen);
		prefix = ", ";
		list++;
	}
	strlcat(buf, " ]\n", buflen);
}

char *badargs[] = {
	"(badargs)",
	NULL
};

void
yamlsize(char *buf, char *key, long pages, size_t buflen)
{
	char tmp[64];

	strlcat(buf, key, buflen);
	strlcat(buf, ": { pages: ", buflen);
	snprintf(tmp, sizeof(tmp), "%ld", pages);
	strlcat(buf, tmp, buflen);
	strlcat(buf, ", bytes: ", buflen);
	snprintf(tmp, sizeof(tmp), "%ld", pages * pagesize);
	strlcat(buf, tmp, buflen);
	strlcat(buf, ", kbytes: ", buflen);
	snprintf(tmp, sizeof(tmp), "%ld", pages * kperpage);
	strlcat(buf, tmp, buflen);
	strlcat(buf, " }\n", buflen);
}

void
printproc(int sd, int index, kvm_t *kd, struct kinfo_proc *kp)
{
	char procbuf[PROCBUFSIZE], login[MAXLOGNAME + 1];
	char tmp[64];
	struct vnode vnode;
	struct inode inode;
	char **av;
	dev_t dev;
	segsz_t ts, ds, ss, rss;
	time_t tm;

	nwrite(sd, "- ", strlen("- "));
	memset(procbuf, 0, PROCBUFSIZE);
	strlcpy(procbuf, "---\n", PROCBUFSIZE);
	yamlint(procbuf, "  index", index, PROCBUFSIZE);

	yamlint(procbuf, "  pid", kp->p_pid, PROCBUFSIZE);
	yamlint(procbuf, "  ppid", kp->p_ppid, PROCBUFSIZE);
	tm = kp->p_ustart_sec;
	yamlstr(procbuf, "  starttime", ctime(&tm), PROCBUFSIZE);
	snprintf(login, sizeof(login), "%.*s", MAXLOGNAME, kp->p_login);
	yamlstr(procbuf, "  login", login, PROCBUFSIZE);
	yamlint(procbuf, "  real_uid", kp->p_ruid, PROCBUFSIZE);
	yamlint(procbuf, "  effective_uid", kp->p_uid, PROCBUFSIZE);
	yamlint(procbuf, "  saved_uid", kp->p_svuid, PROCBUFSIZE);
	yamlint(procbuf, "  real_gid", kp->p_rgid, PROCBUFSIZE);
	yamlint(procbuf, "  effective_gid", kp->p_gid, PROCBUFSIZE);
	yamlint(procbuf, "  saved_gid", kp->p_svgid, PROCBUFSIZE);

	dev = kp->p_tdev;
	if (dev == NODEV) {
		yamlstr(procbuf, "  control_device", "(none)", PROCBUFSIZE);
	} else {
		yamlstr(procbuf, "  ctldev", devname(dev, S_IFCHR), PROCBUFSIZE);
		strlcat(procbuf, "  ctldev_num: [ ", PROCBUFSIZE);
		snprintf(tmp, sizeof(tmp), "%d", major(dev));
		strlcat(procbuf, tmp, PROCBUFSIZE);
		strlcat(procbuf, ", ", PROCBUFSIZE);
		snprintf(tmp, sizeof(tmp), "%d", minor(dev));
		strlcat(procbuf, tmp, PROCBUFSIZE);
		strlcat(procbuf, " ]", PROCBUFSIZE);
	}

	ts = kp->p_vm_tsize;
	ds = kp->p_vm_dsize;
	ss = kp->p_vm_ssize;
	rss = kp->p_vm_rssize,

	yamlsize(procbuf, "  text_size", ts, PROCBUFSIZE);
	yamlsize(procbuf, "  data_size", ds, PROCBUFSIZE);
	yamlsize(procbuf, "  stack_size", ss, PROCBUFSIZE);
	yamlsize(procbuf, "  total_size", ts + ds + ss, PROCBUFSIZE);
	yamlsize(procbuf, "  res_size", rss, PROCBUFSIZE);

	yamlstr(procbuf, "  command", kp->p_comm, PROCBUFSIZE);
	av = kvm_getargv(kd, kp, MAXCALEN);
	if (av == NULL)
		av = badargs;
	yamllst(procbuf, "  argv", av, PROCBUFSIZE);

/*
	if (kp->kp_proc.p_textvp == NULL) {
		yamlstr(procbuf, "  source_file", "(none)", PROCBUFSIZE);
	} else if (kvm_read(kd, (u_long) kp->kp_proc.p_textvp,
	    &vnode, sizeof(struct vnode)) == sizeof(struct vnode)) {
		yamlstr(procbuf, "  source_type", kw(s_vtype, vnode.v_type), PROCBUFSIZE);
		yamlstr(procbuf, "  source_tag_type", kw(s_vtagtype, vnode.v_tag), PROCBUFSIZE);
*/

		/*
		 * It's possible the same code works for FFS, LFS and
		 * EXT2FS too
		 */
/*
		if (vnode.v_tag == VT_UFS && kvm_read(kd, (u_long) vnode.v_data,
		    &inode, sizeof(struct inode)) == sizeof(struct inode)) {
			strlcat(procbuf, "  source_dev_num: [ ", PROCBUFSIZE);
			snprintf(tmp, sizeof(tmp), "%ld", major(inode.i_dev));
			strlcat(procbuf, tmp, PROCBUFSIZE);
			strlcat(procbuf, ", ", PROCBUFSIZE);
			snprintf(tmp, sizeof(tmp), "%ld", minor(inode.i_dev));
			strlcat(procbuf, tmp, PROCBUFSIZE);
			strlcat(procbuf, " ]\n", PROCBUFSIZE);
			yamlstr(procbuf, "  source_device_name",
			    devname(inode.i_dev, S_IFBLK), PROCBUFSIZE);
			yamlint(procbuf, "  source_inode_number",
			    inode.i_number, PROCBUFSIZE);
		}
	}
	strlcat(procbuf, "  ", sizeof(procbuf));
*/

	nswrite(sd, procbuf, strlen(procbuf));
}

void
printprocs(int sd)
{
	kvm_t *kd;
	struct kinfo_proc *kproc;
	segsz_t ts, ds, ss;
	int i, c;
	time_t tm;

	kd = kvm_open(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd == NULL)
		return;

	kproc = kvm_getprocs(kd, KERN_PROC_ALL, 0, sizeof(*kproc), &c);

	nprint(sd, "pagesize: %d", pagesize);
	nprint(sd, "numproc: %d", c);
	nwrite(sd, "--- |\r\n", strlen("--- |\r\n"));
	for (i = 0; i < c; i++) {
		printproc(sd, i, kd, &kproc[i]);
	}

	kvm_close(kd);
}
