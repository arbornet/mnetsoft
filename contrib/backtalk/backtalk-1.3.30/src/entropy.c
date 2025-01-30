/* Copyright 2003, Jan D. Wolter, All Rights Reserved. */

#include "common.h"
#include "entropy.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>

#ifndef NOISE_DEV

/* Various headers for system calls used to get data to feed the entropy pool
 * Lots of fairly os-dependent stuff here.
 */

#if HAVE_SYSINFO
#include <sys/sysinfo.h>
#endif

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#define STATFS statvfs
#else
#define STATFS statfs
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#include <sys/times.h>

#ifdef HAVE_CLOCK_GETTIME
typedef struct timespec timetype;
#define thistime(t)	clock_gettime(CLOCK_REALTIME, &t)
#define thissec(t)	(t).tv_sec
#else
#ifdef HAVE_GETTIMEOFDAY
typedef struct timeval timetype;
#define thistime(t)	gettimeofday(&t,NULL)
#define thissec(t)	(t).tv_sec
#else
typedef long timetype;
#define thistime(t)	(t= time(0L))
#define thissec(t)	t
#endif
#endif

#endif /* ! NOISE_DEV */



/* AS_STR(text,len) - Given 'len' bytes of 8-bit data, generate a ascii string
 * encoding it.  We use this for generating session IDs and what not.  This is
 * a base41 encoding, in which each 2 bytes of data is encoded as 3 characters.
 * Returned value is in dynamic memory.
 */

char *as_str(unsigned char *text, int len)
{
    char *out,*o;
    unsigned char *p, *e;
    unsigned short v;
    static const char b41[] =
	"ABCDEFGHJKLMNPQRSTUVWXYZ3456789aeiohuwyfp";

    out= (char *)malloc(len*3/2+4);

    for (p= (unsigned char *)text, e= p+len, o= out; p < e; p+= 2)
    {
	v= p[0] + ((p+1 >= e) ? 0 : (((unsigned short)p[1]) << 8));

	*(o++)= b41[v % 41];
	*(o++)= b41[(v/41)%41];
	if (p+1 < e)
	    *(o++)= b41[(v/1681)];
    }
    *o= '\0';
    return out;
}


#if 0

/* For the 16 bytes of data that we mostly use, this version doesn't pack the
 * data any denser than the version above, and the character set contains more
 * dubious characters likely to be mangled by various things.  This base 64
 * version is probably faster though.
 */

/* AS_STR(text,len) - Given 'len' bytes of 8-bit data, generate a ascii string
 * encoding it.  We use this for generating session IDs and what not.  The
 * encoding resembles the base64 encoding used in email messages, but we don't
 * insert line breaks and data whose length is not a multiple of 3 are handled.
 * a bit differently.  Each three bytes of input data is encoded with four
 * output characters from a character set of size 64.  Returned value is in
 * dynamic memory.
 */

char *as_str(char *text, int len)
{
    char *out,*o;
    unsigned char *p, *e, p1, p2;
    static const char b64[] =
	"ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789:-!._/";

    out= (char *)malloc(len*4/3+12);

    for (p= (unsigned char *)text, e= p+len, o= out; p < e; p+= 3)
    {
	p1= (p+1 >= e) ? 0 : p[1];
	p2= (p+2 >= e) ? 0 : p[2];

	*(o++)= b64[p[0] >> 2];
	*(o++)= b64[((p[0] & 0x03) << 4) | (p1 >> 4)];
	if (p+1 < e)
	    *(o++)= b64[((p1 & 0x0f) << 2) | (p2 >> 6)];
	if (p+2 < e)
	    *(o++)= b64[p2 & 0x3f];
    }
    *o= '\0';
    return out;
}
#endif


#ifdef NOISE_DEV

/* GET_NOISE() - Return a string of noise in malloced memory.
 * This version uses /dev/random.
 */

#define NOISE_LEN 16	    /* Also change SESSION_ID_LEN if you change this */

char *get_noise()
{
    unsigned char bf[NOISE_LEN], *b= bf;
    int n, i, m, fd;

    if ((fd= open(NOISE_DEV,O_RDONLY)) < 0)
	die("Cannot open "NOISE_DEV);

    /* I hear /dev/urandom sometime returns short counts on some systems.  I
     * don't expect this would be likely to happen with the small amount of
     * data we are reading, but let's be just a bit persistant about getting
     * our 16 bytes worth.
     */
    i= 0;
    n= NOISE_LEN;
    while ((m= read(fd, b, n)) < n)
    {
	if (m < 0)
	    die("Read %d from "NOISE_DEV" failed",i);
	if (i++ > 6) 
	    die("Could not get sufficient data from "NOISE_DEV);
	b+= m;
	n-= m;
	sleep(1);
    }
    close(fd);
    return as_str(bf, NOISE_LEN);
}
#endif /* NOISE_DEV */


#if defined(NOISE_FILE) || defined(NOISE_SQL)

#include "md5.h"

/* ENTROPY POOL FILE
 * For systems without a /dev/urandom device, no good source of entropy exists.
 * So we create a entropy pool of our own, storing it in a file.  Every time
 * we use it, we toss in some low-quality entropy sources.  So that it's state
 * depends not only on values like the current time and process id number, but 
 * on the time and process ID numbers of all past invocations.
 *
 * This entropy pool code is distantly derived from one by Colin Percival.  The
 * original is at http://web.comlab.ox.ac.uk/oucl/work/colin.percival/.
 */

#define POOLMULT 8
#define POOLSIZE (16*POOLMULT)	    /* Must be multiple of 16 */


struct entpool {
    unsigned char pool[POOLSIZE];
    int mixptr;
};

#ifdef NOISE_FILE

/* READ_ENTROPY - Read the current entropy pool from the entropy file.  Returns
 * the file descriptor for the pool, to be passed later to write_entropy().
 */

int read_entropy(unsigned char *pool)
{
    int fd;

    /* Open the file, creating it if necessary */
    if ((fd= open(NOISE_FILE,O_RDWR|O_CREAT,0600)) < 0)
	die("Cannot open or create entropy file "NOISE_FILE);

    read(fd, pool, POOLSIZE);

    return fd;
}


/* WRITE_ENTROPY - Save the new state of the entropy pool.  Closes the file.
 */

void write_entropy(int fd, unsigned char *pool)
{
    lseek(fd, 0L, SEEK_SET);
    write(fd, pool, POOLSIZE);
    close(fd);
}

#endif /* NOISE_FILE */


#ifdef NOISE_SQL

This has not been implemented, and may never be.  If we implement it, we would
supply SQL versions of read_entropy() and write_entropy() here.

#endif /* NOISE_SQL */


/* ADD_ENTROPY - toss some data into the entropy pool.  We slice the input
 * into 16 byte chunks, hash each one with the entire old pool, and X-or the
 * result one of the POOLMULT different 16 byte sections of the pool.
 */

void add_entropy(struct entpool *entropy, unsigned char *buf, int len)
{
    MD5_CTX md5;
    unsigned char digest[16];
    int pos, i;

    for (pos= 0; pos < len; pos+= 16)
    {
	/* Hash 16 bytes (or remainder) of input into old pool contents */
	MD5Init(&md5);
	MD5Update(&md5, buf+pos, (len-pos > 16) ? 16 : (len-pos) );
	MD5Update(&md5, entropy->pool, POOLSIZE);
	MD5Final(digest, &md5);

	/* X-or digest into some 16 byte hunk of the pool */
	for (i= 0; i < 16; i++)
	    entropy->pool[entropy->mixptr+i]^= digest[i];
	entropy->mixptr= (entropy->mixptr+16) % POOLSIZE;
    }
}


/* GET_NOISE() - Return a string of noise in malloced memory.
 * This version uses our own entropy pool, hashing it in with a few other
 * low-quality entropy sources before returning it.  (We don't want to trust
 * wholely in the pool because there is a risk that someone might have gotten
 * read access to it.
 */

char *get_noise()
{
    struct entpool entropy;
    unsigned char digest[16];
    long ticks;
    struct tms ts;
    MD5_CTX md5;
    int fd;
    pid_t pid= getpid();
    timetype tm;
#ifdef HAVE_SYSINFO
    struct sysinfo si;
#else
#ifdef HAVE_STATVFS
    struct STATFS fs;
#endif
#endif

    /* Read our finest-resolution clock and set mixptr to a random value */
    thistime(tm);
    entropy.mixptr= (thissec(tm) % POOLMULT) * 16;

    /* Load in our entropy pool */
    fd= read_entropy(entropy.pool);

    /* Stir in some stuff */
    add_entropy(&entropy, (unsigned char *)&tm, sizeof(tm));
    add_entropy(&entropy, (unsigned char *)&pid, sizeof(pid));

    /* Save the stirred pool */
    write_entropy(fd, entropy.pool);


    /* Hash some low quality junk with pool to make our key */
    MD5Init(&md5);

    /* - The entire entropy pool */
    MD5Update(&md5, entropy.pool, POOLSIZE);

    /* - The current time (after the read/write of the pool) */
    thistime(tm);
    MD5Update(&md5, (unsigned char *)&tm, sizeof(tm));

    /* - Some process execution times - not really very variable */
    ticks= (long)times(&ts);
    MD5Update(&md5, (unsigned char *)&ticks, sizeof(ticks));
    MD5Update(&md5, (unsigned char *)&ts, sizeof(ts));

#ifdef HAVE_SYSINFO
    /* This is nice, but exists only on Linux which has /dev/urandom, so why
     * do we even bother?
     */
    sysinfo(&si);
    MD5Update(&md5, (unsigned char *)&si, sizeof(si));
#else
#ifdef HAVE_STATVFS
    /* This isn't very good - disk usage even on /var doesn't change all that
     * fast, but it's something.  Some older systems don't even have /var
     * in which case this will be pretty much good for nothing.
     */
    STATFS("/var",&fs);
    MD5Update(&md5, (unsigned char *)&fs, sizeof(fs));
#endif
#endif
    MD5Final(digest, &md5);

    /* Convert digest to ascii and return it */
    return as_str(digest,16);
}


#ifdef MAKE_NOISE

/* MAKE_NOISE - This can be called from time to time with additional stuff to
 * stir into the entropy pool.
 */

void make_noise(char *stuff1, int len1, char *stuff2, int len2)
{
    int fd;
    timetype tm;

    if (len1 < 0) len1= strlen(stuff1);
    if (len2 < 0) len2= strlen(stuff2);
    if (len1 > POOLSIZE/2) len1= POOLSIZE/2;
    if (len2 > POOLSIZE/2) len2= POOLSIZE/2;

    /* Load in our entropy pool */
    fd= read_entropy(entropy.pool);
    thistime(tm);
    entropy.mixptr= (thisec(tm) % POOLMULT) * 16;

    /* Stir in passed in stuff */
    add_entropy(&entropy, (unsigned char *)stuff1, len1);
    if (stuff2 != NULL)
	add_entropy(&entropy, (unsigned char *)stuff2, len2);

    /* Stir in timestamp */
    add_entropy(&entropy, (unsigned char *)&tm, sizeof(tm));

    /* Save the stirred pool */
    write_entropy(fd, entropy.pool);
}
#endif /* MAKE_NOISE */

#endif /* NOISE_FILE || NOISE_SQL */
