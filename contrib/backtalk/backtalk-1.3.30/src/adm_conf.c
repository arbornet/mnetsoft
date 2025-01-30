/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#include <ctype.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "adm_conf.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* CHECK_CONF_NAME - Do syntax checks on a conference name.  This will be the
 * final component of the path and it will be one of the names the conference
 * can be accessed by, so it has to be a legal unix path name, and it has to
 * fit into fit into a conflist.  Since Backtalk likes conference names to
 * be case insensitive, we want it all in lowercase.  So we allow only lower-
 * case letters, digits, and a few symbols.  This is more draconian than we
 * really need to be, but I think this makes sense.  Returns 1 if the name is
 * unacceptable.
 *
 * Symbols not allowed:
 *  Would mess up conflist file:    : _
 *  Not legal in Unix filenames:    /
 *  Clumsy to type in shell:        &!$"';()*?<>{}[]
 *
 * Symbols not allowed as first character:
 *  Makes hidden directory:         .
 *  Clumsy to type in shell:        -
 */

static int check_conf_name(char *name)
{
    static char *ok_sym= "-.+@#%";
    static char *bad_first= "-.";

    if (name == NULL || name[0] =='\0' || strchr(bad_first,name[0]) != NULL)
	return 1;

    for (; *name != '\0'; name++)
    {
	if (!isascii(*name) ||
	    (!islower(*name) && !isdigit(*name) &&
	     strchr(ok_sym,*name) == NULL))
	    return 1;
    }
    return 0;
}


/* CHECK_CONF - Given a conference name, a parent directory name and a
 * participation file name, check that they are usable.  Returns an error
 * string if something is bad.
 *
 * Checks for:
 * - conference name null or empty.
 * - conference name has anything other than lowercase or digits in it.
 * - parent directory does not exist.
 * - parent directory is not writable.
 * - parent directory already constains something with "name".
 * - conference name matchs an alias in CONFLIST.
 * - another conference has pfile name.
 *
 * This isn't as efficient as it could be, since it will read the same
 * conference config file more than once if the conference has more than one
 * alias in the CONFLIST file.  But this operation isn't done often enough
 * to make it worth avoiding.
 */

char *check_conf(char *name, char *loc, char *pfile)
{
    FILE *fp, *cfp;
    static char buf[BFSZ];
    struct stat st;
    char dir[BFSZ];
    char *d, *b;
    int err= 0;

    /* Check syntax of conference name */
    if (check_conf_name(name))
    {
	sprintf(buf,"Illegal conference name: `%.100s`",name);
	return buf;
    }

    /* Check if parent directory exists */
    if (stat(loc,&st) == -1)
    {
	sprintf(buf,"Directory %.100s is not accessible",loc);
	return buf;
    }

    /* Check if parent directory is writable */
    if (!(st.st_uid == CFADM_UID && (st.st_mode&0200)) &&
	!(st.st_gid == getegid() && (st.st_mode&0020)) &&
	!(st.st_mode & 0002))
    {
	sprintf(buf,"Directory %.100s is not writable",loc);
	/*sprintf(buf,"Directory %.100s is not writable (u%d-%d g%d-%d m%d)",
	loc,st.st_uid,CFADM_UID,st.st_gid,getegid(),st.st_mode);*/
	return buf;
    }

    /* Construct future directory path */
    sprintf(dir,"%.100s%.100s",loc,name);

    if (stat(dir,&st) != -1)
    {
	sprintf(buf,"Directory %.50s already exists",dir);
	return buf;
    }

    if ((fp= fopen(CONFLIST,"r")) == NULL)
	die("could not open "CONFLIST);

    /* Read and check magic code number */
    if (fgets(buf,BFSZ,fp) == NULL || strcmp(buf,"!<hl01>\n"))
	die("conference list "CONFLIST" has bad magic number");

    /* Read and ignore default conference path - skipping comments */
    for (;;)
    {
	if (fgets(buf,BFSZ,fp) == NULL)
	    die("conference list "CONFLIST" too short");
	if (buf[0] != '#') break;
    }

    /*  Search list of conferences */
    while (fgets(buf,BFSZ,fp) != NULL)
    {
	/* Skip comments and blank lines */
	if (buf[0] == '#' || buf[0] == '\n') continue;

	/* Parse apart line */
	if ((d= strchr(buf,':')) == NULL)
	    die("bad line in conference list "CONFLIST);
	d++;
	if ((b= strchr(d,'\n')) == NULL)
	    die("bad line in conference list "CONFLIST);
	*b= '\0';

	/* Construct path name of conference config file */
	if (d[0] == '%')
	    sprintf(dir,BBS_DIR"/%.100s/config",d+1);
	else
	    sprintf(dir,"%.100s/config",d);

	/* If can't open the config file, don't worry about it */
	if ((cfp= fopen(dir,"r")) == NULL)
	    continue;

	/* Check if our name matched the pattern for the conference */
	if (match(name, buf))
	{
	    sprintf(buf,"Name %.50s in use by another conference",name);
	    return buf;
	}


	/* Check config file version line */
	if (fgets(buf,BFSZ,cfp) == NULL || strcmp(buf,"!<pc02>\n"))
	    continue;

	/* Get participation file name */
	if (fgets(buf,BFSZ,cfp) == NULL || (b= strchr(buf,'\n')) == NULL)
	    continue;
	*b = '\0';
	fclose(cfp);

	/* Check for uniqueness */
	if (!strcmp(buf,pfile))
	{
	    sprintf(buf,"Participation file name %.50s in use by another conference",pfile);
	    return buf;
	}
    }
    fclose(fp);
    return NULL;
}


/* COMPRESS_DIR - Given a fully written out directory, replace a leading
 * % sign with BBS_DIR.  The result is written into out.  Picospan understands
 * this.  Yapp does not.
 */

void compress_dir(char *dir, char *out)
{
#ifndef YAPP_COMPAT
    int len;

    len= strlen(BBS_DIR);
    while (BBS_DIR[len-1] == '/')
    	len--;

    if (dir[len] == '/' && !strncmp(dir,BBS_DIR,len))
    {
	do { len++; } while (dir[len] == '/');
	sprintf(out,"%%%.100s",dir+len);
    }
    else
	strncpy(out,dir,100);
#endif /* YAPP_COMPAT*/
}


/* ADD_CONFLIST - Add an entry to the conference list.  Try collapsing the
 * directory name, if possible.
 */

void add_conflist(char *alias, char *dir)
{
    int len;
    char buf[BFSZ];
    int fd;

    /* Construct the line */
    if ((len= strlen(alias)) > 100) len= 100;
    strncpy(buf,alias, len);
    buf[len]= ':';
    compress_dir(dir,buf+len+1);
    strcat(buf,"\n");

    if ((fd= open(CONFLIST,O_WRONLY|O_APPEND)) < 0)
    	die("Cannot append to "CONFLIST);

    write(fd,buf,strlen(buf));

    close(fd);
}


/* DEL_CONFLIST - Delete all entries with the given directory name from the
 * conflist.  No locking.
 */

void del_conflist(char *dir)
{
    struct stat st;
    char buf[BFSZ];
    char cdir[BFSZ];
    char *mem, *s, *n, *c;
    FILE *fp;
    int dlen;
    size_t sz;

    compress_dir(dir,cdir);
    dlen= strlen(cdir);

    if ((fp= fopen(CONFLIST,"r+")) == NULL || fstat(fileno(fp),&st))
    	die("Cannot open to "CONFLIST);

    /* Load the whole file into memory in one gulp */
    mem= (char *)malloc(st.st_size+1);
    fread(mem, 1, st.st_size, fp);
    mem[st.st_size]= '\0';

    /* rewind the file */
    fseek(fp,0L,0);

    /* rewrite the file, omitting lines that match */
    s= mem;
    sz= 0;
    while (*s != '\0')
    {
    	if ((n= strchr(s,'\n')) == NULL)
    	    die(CONFLIST" does not end with a newline?\n");
    	if (s[0] == '#' || (c= strchr(s,':')) == NULL ||
	    c[dlen+1] != '\n' || strncmp(c+1,cdir,dlen))
    	{
    	    /* Directory doesn't match - type this line */
    	    while (s <= n)
    	    {
    	    	putc(*(s++),fp);
    	    	sz++;
	    }
        }
        s= n+1;
    }
    ftruncate(fileno(fp), sz);
    fclose(fp);
    free(mem);
}


/* MAKE_CONFDIR - Create a conference directory, with the indexdir subdirectory
 * and the config file.  This does no sanity checking on its arguments.
 */

void make_confdir(char *dir, char *pfile, char *fw, int mode, char *title)
{
int fd;
char path[BFSZ];
int dirlen;

    umask(0);

    /* Create conference subdirectory */
    if (mkdir(dir,0755))
	die("Cannot create directory %.100s",dir);

    /* Copy directory name into a larger buffer */
    strcpy(path,dir);
    dirlen=strlen(dir);
    path[dirlen++]= '/';

    /* Create index subdirectory */
    strcpy(path+dirlen,INDEX_SUBDIR);
    if (mkdir(path,0755))
	die("Cannot create directory %.100s",dir);

    /* Make the config file */
    strcpy(path+dirlen,"config");
    if ((fd= open(path,O_WRONLY|O_CREAT|O_EXCL,0644)) < 0)
    	die("Cannot create file %.100s",path);

    write(fd,"!<pc02>\n",8); 
    write(fd,pfile,strlen(pfile));
    write(fd,"\n0\n",3); 
    write(fd,fw,strlen(fw));
    write(fd,"\n",1);
    if (mode != 0 || title != NULL)
    {
    	char bf[BFSZ];
    	sprintf(bf,"%d\n",mode);
    	write(fd,bf,strlen(bf));
    	if (title != NULL)
    	{
#ifdef YAPP_COMPAT
	    /* this line used for mail addresses by Yapp */
	    write(fd,"\n",1);
#endif /* YAPP_COMPAT */
    	    write(fd,title,strlen(title));
	    write(fd,"\n",1);
    	}
    }
    close(fd);
}


/* MAKE_PART_NAME - Given a conference name, generate the normal participation
 * file name in malloc'ed memory.
 */

char *make_part_name(char *conf)
{
    int len= strlen(conf);
    char *new= (char *)malloc(len+5);

    new[0]= '.';
    strcpy(new+1, conf);
    new[len+1]= '.';
    new[len+2]= 'c';
    new[len+3]= 'f';
    new[len+4]= '\0';

    return new;
}
