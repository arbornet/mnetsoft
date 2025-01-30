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
#include <unistd.h>

enum
{
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
save_args(char *buf, char **arg)
{
	int i, j;
	char *p;

	p = "";
	strlcpy(buf, p, MAXCLEN + 1);
	while (*arg != NULL) {
		strlcat(buf, p, MAXCALEN + 1);
		strlcat(buf, *arg++, MAXCALEN + 1);
		p = " ";
	}
}

void
printprocs(int sd)
{
	kvm_t *kd;
	struct kinfo_proc *kproc;
	segsz_t ts, ds, ss;
	dev_t dev;
	int i, cnt;
	time_t tm;
	struct vnode vnode;
	struct inode inode;
	char **av, args[MAXCALEN + 1];

	long pagesize = sysconf(_SC_PAGESIZE);
	long kpagesize = pagesize / 1024;

	if (getuid() != 0) {
		printf("You must be root to run this\n");
		exit(1);
	}
	nprint(sd, "pagesize=%d\n", pagesize);

	kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvmerror");
	if (kd == NULL)
		return;

	kproc = kvm_getprocs(kd, KERN_PROC_ALL, 0, sizeof (*kproc), &cnt);
	printf("NUMBER OF PROCESSES=%d\n", cnt);
	fflush(stdout);

	for (i = 0; i < cnt; i++) {
		printf("%d:\n", i);
		printf("   PID=%d\n", kproc[i].kp_proc.p_pid);
		printf("   PARENT PID=%d\n", kproc[i].kp_eproc.e_ppid);

		/* I believe this is always null terminated */
		printf("   COMMAND=%s\n", kproc[i].kp_proc.p_comm);

		av = kvm_getargv(kd, kproc + i, MAXCALEN);
		if (av == NULL) {
			printf("   COMMAND LINE=(not available)\n");
		} else {
			save_args(args, av);
			printf("   COMMAND LINE=%s\n", args);
		}

		tm = kproc[i].kp_eproc.e_pstats.p_start.tv_sec;
		printf("   START TIME=%s", ctime(&tm));

		printf("   LOGIN=%.*s\n", MAXLOGNAME, kproc[i].kp_eproc.e_login);
		printf("   REAL UID=%d\n", kproc[i].kp_eproc.e_pcred.p_ruid);
		printf("   EFFECTIVE UID=%d\n", kproc[i].kp_eproc.e_ucred.cr_uid);
		printf("   SAVED UID=%d\n", kproc[i].kp_eproc.e_pcred.p_svuid);
		printf("   REAL GID=%d\n", kproc[i].kp_eproc.e_pcred.p_rgid);
		printf("   EFFECTIVE GID=%d\n", kproc[i].kp_eproc.e_ucred.cr_gid);
		printf("   SAVED GID=%d\n", kproc[i].kp_eproc.e_pcred.p_svgid);


		dev = kproc[i].kp_eproc.e_tdev;
		if (dev == NODEV) {
			printf("   NO CONTROL DEVICE\n");
		} else {
			printf("   CONTROL DEVICE NUMBER=(%d,%d)\n", major(dev), minor(dev));
			printf("   CONTROL DEVICE NAME=%s\n", devname(dev, S_IFCHR));
		}

		ts = kproc[i].kp_eproc.e_vm.vm_tsize;
		printf("   TEXT SIZE=%d PAGES, %d BYTES, %d K\n",
		       ts, ts * pagesize, ts * kpagesize);
		ds = kproc[i].kp_eproc.e_vm.vm_dsize;
		printf("   DATA SIZE=%d PAGES, %d BYTES, %d K\n",
		       ds, ds * pagesize, ds * kpagesize);
		ss = kproc[i].kp_eproc.e_vm.vm_ssize;
		printf("   STACK SIZE=%d PAGES, %d BYTES, %d K\n",
		       ss, ss * pagesize, ss * kpagesize);
		printf("   TOTAL SIZE=%d PAGES, %d BYTES, %d K\n", (ts + ds + ss),
		     (ts + ds + ss) * pagesize, (ts + ds + ss) * kpagesize);
		printf("   RESIDENT SIZE=%d PAGES, %d BYTES, %d K\n",
		       kproc[i].kp_eproc.e_vm.vm_rssize,
		       kproc[i].kp_eproc.e_vm.vm_rssize * pagesize,
		       kproc[i].kp_eproc.e_vm.vm_rssize * kpagesize);

		if (kproc[i].kp_proc.p_textvp == NULL) {
			printf("   SOURCE FILE=(none)\n");
		} else {
			if (kvm_read(kd, (u_long) kproc[i].kp_proc.p_textvp,
				     &vnode, sizeof(struct vnode)) != sizeof(struct vnode)) {
				printf("Could not read vnode\n");
				continue;
			}
			printf("   SOURCE TYPE=%s\n", kw(s_vtype, vnode.v_type));
			printf("   SOURCE TAG TYPE=%s\n", kw(s_vtagtype, vnode.v_tag));

			/*
			 * It's possible the same code works for FFS, LFS and
			 * EXT2FS too
			 */
			if (vnode.v_tag == VT_UFS) {
				if (kvm_read(kd, (u_long) vnode.v_data,
					     &inode, sizeof(struct inode)) != sizeof(struct inode)) {
					printf("Could not read inode\n");
					continue;
				}
				printf("   SOURCE DEVICE NUMBER=(%d,%d)\n",
				    major(inode.i_dev), minor(inode.i_dev));
				printf("   SOURCE DEVICE NAME=%s\n",
				       devname(inode.i_dev, S_IFBLK));
				printf("   SOURCE INODE NUMBER=%d\n", inode.i_number);
			}
		}
	}

	kvm_close(kd);

	exit(0);
}
