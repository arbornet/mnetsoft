/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#if HAVE_SETRLIMIT
#include <sys/resource.h>
#endif /* HAVE_SETRLIMIT */

#include "udb.h"
#include "file.h"
#include "tag.h"
#include "str.h"
#include "vstr.h"
#include "readfile.h"
#include "last_log.h"
#include "partdir.h"
#include "group.h"
#include "partfile.h"
#include "set_uid.h"

#define wst(n,v) wrtstrtag(stdout,n,v,FALSE)
#define wit(n,v) wrtinttag(stdout,n,v,FALSE)

#include <pwd.h>
#include <string.h>

#ifdef HAVE_DIRENT_H
#include <dirent.h>
typedef struct dirent direntry;
#else
typedef struct direct direntry;
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#ifdef MTRACE
#include <mcheck.h>
#endif

int warn_missing= TRUE;

void init_cmd(char *progname, int check)
{
#if HAVE_SETRLIMIT
    struct rlimit rlim;

    /* Don't dump core (dumps could contain sensitive data) */
    rlim.rlim_cur = rlim.rlim_max = 0;
    (void)setrlimit(RLIMIT_CORE, &rlim);
#endif

    if (getuid() == 0)
	set_root_uid();
    else
	set_httpd();
    if (check && (geteuid() != SERVER_UID))
    {
	fprintf(stderr,"%s: This program should be installed setuid "
	   "to uid=%d.\n",progname,SERVER_UID);
	exit(1);
    }

    set_cfadm();
    if (geteuid() != CFADM_UID)
    {
	fprintf(stderr,"%s: You are not the conference adminstrator \n",
	    progname);
	exit(1);
    }
}


/* PARTICIPATION FILE DIRECTORY - Given a user, return the full path of
 * the directory containing his .backtalk file and participation files.
 */

void pf_dir(char *bf, struct passwd *pw)
{
#ifdef PART_DIR
    bbspartpath(bf, pw->pw_name, "");
#else
    homepartpath(bf, pw->pw_dir, "");
#endif
}


/* RC FILE DIRECTORY - Given a user, return the full path of
 * the directory containing his .cfonce and .cfrc files.
 */

int rc_dir(char *bf, struct passwd *pw)
{
    homepartpath(bf, pw->pw_dir, "");
}


char **grplist;	/* An array of pointers to group names, indexed by group id */
int maxgrp;     /* The maximum group index */
int grpalloc= 0;/* The number of elements allocated for the group array */


/* ADD_GROUP - Add the group with the given name into the array structure
 * above.
 */

void add_group(char *grpname)
{
    int i;
    int grpid= groupid(grpname);

    if (grpid < 0) return;

    /* Allocate storage, if needed */
    if (grpid >= grpalloc)
    {
	if (grpalloc == 0)
	{
	    grpalloc= grpid + 10;
	    grplist= (char **)malloc(grpalloc*sizeof(char *));
	}
	else
	{
	    grpalloc= grpid + 20;
	    grplist= (char **)realloc(grplist,grpalloc*sizeof(char *));
	}
    }

    if (grpid > maxgrp)
    {
	/* Fill in any gap with NULL pointers */
	for (i= maxgrp + 1; i < grpid; i++)
	    grplist[i]= NULL;

	maxgrp= grpid;
    }

    /* Save the new group */
    grplist[grpid]= strdup(grpname);
}



/* GET_USER_GROUPS - return a comma-separated list of groups that the user
 * is in, not including the the primary group passed in as an argument.
 * Value is returned in malloced memory.  Return NULL if the user is in no
 * other groups.
 */

char *get_user_groups(char *userid, int prime_gid)
{
    int i;
    VSTRING out;
    vs_new(&out, 0);

    for (i= 0; i < maxgrp; i++)
    {
	if (i != prime_gid &&
	    grplist[i] != NULL &&
	    ingroupno(i, prime_gid, userid))
	{
	    if (out.p > out.begin) *vs_inc(&out)= ',';
	    vs_cat(&out,grplist[i]);
	}
    }
    if (out.p > out.begin)
	return vs_return(&out);
    else
	return NULL;
}


struct plist {
    char *pf;		/* Name of the participation file */
    char *cf;		/* Name of the conference */
    int cflen;		/* Length of minimum abbreviation of name */
    char *dr;		/* Path of the conference directory */
    struct plist *next;
} *partlist;


/* READCFLIST - Load the conflist into a linked list of plist structures.
 * We skip any duplicate entries.  Conference name is taken as shortest
 * legal abbreviation that would not be shadowed by a previous abbreviation.
 * pf fields are not initialized.
 */

void readcflist()
{
    FILE *fp;
    char buf[BFSZ], *p, *q, *dir;
    struct plist *pl;
    int first= TRUE, clen;

    if ((fp= fopen(CONFLIST,"r")) == NULL)
	die("Could not open "CONFLIST);

    if (fgets(buf,BFSZ,fp) == NULL || strcmp(buf,"!<hl01>\n"))
	die("conference list "CONFLIST" has bad magic number");

    while (fgets(buf,BFSZ,fp) != NULL)
    {
	if (buf[0] == '#') continue;
	if (first) {first= FALSE; continue;}	/* skip first line */
	if ((p= firstin(buf,":")) == NULL)
	{
	    fclose(fp);
	    die("Bad line in "CONFLIST": '%s'", buf);
	}
	*(p++)= '\0';
	*(firstin(p,"\n"))= '\0';

	/* Find _ in name.  Set clen to minimum legal abbreviation */
	if ((q= strchr(buf,'_')) == NULL)
	    clen= strlen(buf);
	else
	{
	    clen= q-buf;
	    /* Remove _ from name */
	    for ( ; *q != '\0'; q++)
		q[0]= q[1];
	}

	/* Expand % in path name */
	if (p[0] == '%')
	{
	    dir= (char *)malloc(strlen(BBS_DIR)+strlen(p));
	    strcpy(dir,BBS_DIR);
	    strcat(dir,p+1);
	}
	else
	    dir= strdup(p);

	/* Check if we already have entry with this directory name.
	 * While scanning, expand conference name abbreviation to avoid
	 * shadowing. */
	for (pl= partlist; pl != NULL && strcmp(pl->dr,dir); pl= pl->next)
	{
	    while (!strncmp(pl->cf, buf, clen))
	    {
		if (buf[clen] == '\0')
		{
		    fprintf(stderr,"Conference %s(%d) shadows %s(%s) in "
			CONFLIST, pl->cf, pl->dr, buf, dir);
		    goto nextline;
		}
		clen++;
	    }
	}

	/* Add it in if not */
	if (pl == NULL)
	{
	    pl= (struct plist *)malloc(sizeof(struct plist));
	    pl->cf= strdup(buf);
	    pl->dr= dir;
	    pl->next= partlist;
	    partlist= pl;
	}

nextline:;
    }
    fclose(fp);
    if (first)
	die("conference list "CONFLIST" is too short");
}


/* MKPARTLIST - Get a list of all participation files with conference
 * names.  Goes into global partlist variable.
 */

void mkpartlist()
{
    struct plist *pl;
    char buf[BFSZ+1];
    FILE *fp;

    /* Get list from conflist file */
    partlist= NULL;
    readcflist();

    /* Look up participation file name for each.  This is just second line
     * of config file.
     */
    for (pl= partlist; pl != NULL; pl= pl->next)
    {
	sprintf(buf,"%s/config",pl->dr);
	if ((fp= fopen(buf,"r")) == NULL)
	    die("Cannot open conference config file %d",buf);
	if (fgets(buf,BFSZ,fp) == NULL || strcmp(buf,"!<pc02>\n"))
	    die("Bad config file version number: %s", buf);
	if (fgets(buf,BFSZ,fp) == NULL)
	    die("conference configuration file %s is missing name of "
		"participation file",buf);
	*firstin(buf,"\n")= '\0';
	pl->pf= strdup(buf);
	fclose(fp);
    }
}


/* FINDPART - Find the partlist entry corresponding to a give participation
 * file name.
 */

struct plist *findpart(char *pfname)
{
    struct plist *pl;

    for (pl= partlist; pl != NULL; pl= pl->next)
	if (!strcmp(pl->pf,pfname))
	    return pl;
    return NULL;
}


/* XMLQUOTE - Expand " to &quot; and so forth.  We'd use sysformat() but
 * that is so riddled with references to the dictionary and the stack that
 * it doesn't embed well in stand alone programs.  So we write a little
 * clean version.  Returns the expanded string in dynamic memory, which
 * the caller must free.
 */

char *xmlquote(char *s)
{
    VSTRING out;

    vs_new(&out, 0);

    for (; *s != '\0'; s++)
	switch (*s)
	{
	    case '<':  vs_cat_n(&out,"&lt;",  4); break;
	    case '>':  vs_cat_n(&out,"&gt;",  4); break;
	    case '&':  vs_cat_n(&out,"&amp;", 5); break;
	    case '"':  vs_cat_n(&out,"&quot;",6); break;
	    case '\'': vs_cat_n(&out,"&apos;",6); break;
	    default:   *vs_inc(&out)= *s;         break;
	}
    return vs_return(&out);
}


/* WRITEHASHDATA - Write out tag lines for everything in the given hash */

void writehashdata(HashTable *hash)
{
    HashEntry *h;
    HashSearch hs;

    for (h= FirstHashEntry(hash, &hs); h != NULL; h= NextHashEntry(&hs))
	wrttag(stdout, h->key, &(h->t));
}


/* WRTTAG - Duplicate of the version in file.c so we don't have to link in
 * file.c
 */

void wrttag(FILE *fp, char *name, Token *t)  /* DUPLICATE */
{
    if (type(*t) == TK_STRING)
	wrtstrtag(fp,name,sval(*t),FALSE);
    else if (type(*t) == TK_INTEGER)
	wrtinttag(fp,name,ival(*t),FALSE);
    else
	die("%s is not a string or integer variable", name);
}


/* WRITEFILE - Given a full path and an identifier for a file, write it
 * out surrounded by <file> tags.  If the file is not readable, write nothing.
 * Quote file and make sure it ends with a newline.
 */

void writefile(char *fname, char *what)
{
    int i;
    char *content, *quoted;

    content= read_file(fname,FALSE);
    if (content == NULL || *content == '\0')
	return;
    printf("<file name=\"%s\">\n", what);
    quoted= xmlquote(content);
    free(content);
    fputs(quoted, stdout);
    i= strlen(quoted);
    if (quoted[i-1] != '\n') putchar('\n');
    free(quoted);
    printf("</file>\n", what);
}


/* WRITEPF - Write out a description of the participation file for the given
 * user with the given name.  The path of the user's participation file
 * directory is given.
 */

void writepf(struct passwd *pw, char *pfpath, char *pfname)
{
    struct plist *pl;
    struct pfc pf;
    struct pfe *item;
    char *f;

    /* See if we have a conference for this file first */
    if ((pl= findpart(pfname)) == NULL)
    {
	if (warn_missing)
	    fprintf(stderr,"Skipping participation file %s/%s - "
		"not for any known conference\n", pfpath, pfname);
	return;
    }

    /* Make a participation structure for this conference */
    pf.snooping= TRUE;
    strcpy(pf.part_name,pfpath);
    strcat(pf.part_name,"/");
    strcat(pf.part_name,pfname);
    pf.user= pw->pw_name;
    pf.conf= pl->cf;
    pf.confdir= pl->dr;

    /* Load it in */
    if (loadpart(&pf))
    {
	fprintf(stderr,"Cannot load participation file %s/%s\n",
		pfpath, pfname);
	freepart(&pf);
	closepf(&pf);
	return;
    }

    /* Print it out */
    printf("<participation conf=\"%s\">\n", f= xmlquote(pl->cf));
    free(f);

    if (pf.alias != NULL || pf.conf_etc != NULL)
	printf("<data>\n");
    if (pf.alias != NULL)
	wst("alias",pf.alias);
    if (pf.conf_etc != NULL)
	writehashdata(pf.conf_etc);
    if (pf.alias != NULL || pf.conf_etc != NULL)
	printf("</data>\n");

    for (item= pf.pflist; item != NULL; item= item->next)
    {
	printf("<item number=\"%d\">\n", item->item_no);
	wit("lastresp", item->resp_no);
	wit("lastdate",item->date);
	if (item->forgotten) wit("forgotten",1);
	if (item->favorite) wit("favorite",1);
	if (item->my_title != NULL) wst("title",item->my_title);
	if (item->etc != NULL) writehashdata(item->etc);
	printf("</item>\n");
    }

    printf("</participation>\n");

    /* Deallocate stuff */
    freepart(&pf);
    closepf(&pf);
}


void dump_user(struct passwd *pw, int dump_pass, int save_part)
{
    char *f, *g;
    rtag *rt;
    char rc[BFSZ];
#ifdef PART_DIR
    char pf[BFSZ];
#else
    char *pf;
#endif
    int rclen,pflen;

    if (pw == NULL) return;

    printf("<user login=\"%s\">\n",
	f= xmlquote(pw->pw_name), pw->pw_uid, pw->pw_gid);
    free(f);
    printf("<data>\n");
    if (pw->pw_gecos != NULL && *pw->pw_gecos != '\0')
	wst("usrname",pw->pw_gecos);
    wit("usrid",pw->pw_uid);
    wst("grpname",groupname(pw->pw_gid));
    g= get_user_groups(pw->pw_name, pw->pw_gid);
    if (g != NULL)
    {
	wst("grps",g);
	free(g);
    }
    if (dump_pass)
    {
	int status;
	char *p= pw->pw_passwd;

	if ( p == NULL || *p == '\0')
	    p= getpassword(pw->pw_name, FALSE);

	if ( p != NULL && *p != '\0')
	{
	    if (*p == '?')
	    {
		status= 1; p++;
	    }
	    else if (*p == '*')
	    {
		status= 2; p++;
	    }
	    else
		status= 0;

	    wst("encpasswd",p);
	    if (status != 0) wit("usrstat", status);
	}
    }
    wst("dirpath",pw->pw_dir);

    /* Get last log time */
#ifdef LASTLOG_FILE
    wit("laston",get_lastlog(pw->pw_uid));
#endif

    /* Construct directory path for .cfonce and .cfrc */
    rc_dir(rc, pw);
    rclen= strlen(rc);
    /* Construct directory path for participation files and .backtalk */
#ifdef PART_DIR
    homepartpath(pf, pw->pw_dir, "");
    pflen= strlen(pf);
#else
    pf= rc;
    pflen= rclen;
#endif

    /* Load .backtalk file */
    strcpy(pf+pflen, ".backtalk");
    if ((rt= open_tag(pf,TRUE)) != NULL)
    {
	int code;
	char *var, *val;

	while ((code= get_tag(rt, &var, &val)) > 0)
	{
	    switch (code)
	    {
		case 1: wst(var,val); break;
		case 2: wit(var,atoi(val)); break;
		case 3: wrtstrtag(stdout,var,val,TRUE); break;
		case 4: wrtinttag(stdout,var,atoi(val),TRUE); break;
	    }
	}
	close_tag(rt);
    }
    printf("</data>\n");
    
    /* Load .cflist file */
    strcpy(pf+pflen, ".cflist");
    writefile(pf, ".cflist");

    /* Load .cfonce file */
    strcpy(rc+rclen, ".cfonce");
    writefile(rc, ".cfonce");

    /* Load .cfrc file */
    strcpy(rc+rclen, ".cfrc");
    writefile(rc, ".cfrc");

    if (save_part)
    {
	DIR *dp;
	direntry *file;

	pf[pflen]= '\0';
	if ((dp= opendir(pf)) != NULL)
	{
	    while ((file= readdir(dp)) != NULL)
	    {
		int i;
		char *f= file->d_name;

		if (f[0] != '.') continue;
		i= strlen(f);
		if (i >= 5 && f[i-3] == '.' &&
			      f[i-2] == 'c' &&
			      f[i-1] == 'f')
		    writepf(pw, pf, f);
	    }
	    closedir(dp);
	}
    }
    printf("</user>\n");
}


int main(int argc, char **argv)
{
    struct passwd *pw;
    int dump_pass= TRUE;
    int save_part= TRUE;
    int check_uid= TRUE;
    int verbose= FALSE;
    struct usl {
	char *u;
	struct usl *next;
    } *u, *last= NULL, *userlist= NULL;

    int i,j;

#ifdef MTRACE
    mtrace();
#endif

    for (i= 1; i < argc; i++)
    {
	if (argv[i][0] == '-')
	    for (j= 1; argv[i][j] != '\0'; j++)
		switch (argv[i][j])
		{
		    case 'm':  warn_missing= FALSE; break;
		    case 'p':  save_part= FALSE; break;
		    case 'v':  verbose= TRUE; break;
		    case 'u':  check_uid= FALSE; break;
		    case 'P':  dump_pass= FALSE; break;
		    default: goto usage;
		}
	else
	{
	    /* Insert user into user name list */
	    u= (struct usl *)malloc(sizeof(struct usl));
	    u->u= argv[i];
	    u->next= NULL;
	    if (last == NULL)
		userlist= u;
	    else
		last->next= u;
	    last= u;
	}
    }

    init_cmd(argv[0],check_uid);

    if (save_part)
    {
	/* Build a list of conference names/directories that go with
	 * each participation file name.
	 */
	mkpartlist();
    }

    /* Initialize the group list structure */
    for_groups(add_group);

    printf("<userdata date=\"%ld\" version=\"%d.%d.%d\">\n", time(0L),
	    VERSION_A, VERSION_B, VERSION_C);

    /* Looop through the users */
    if (userlist == NULL)

	/* All users */
	for (pw= walkdb(0); pw != NULL; pw= walkdb(1))
	{
	    if (verbose) fprintf(stderr,"%s\n",pw->pw_name);
	    dump_user(pw, dump_pass, save_part);
	}
    
    else

	/* Just listed users */
	for (u= userlist; u != NULL; u= u->next)
	{
	    struct passwd *pw;
	    if (verbose) fprintf(stderr,"%s\n",u->u);
	    pw= getdbnam(u->u);
	    if (pw == NULL) fprintf(stderr, "user %s not found\n", u->u);
	    dump_user(pw, dump_pass, save_part);
	}

    printf("</userdata>\n");

    exit(0);
usage:
    fprintf(stderr,"usage: %s [-pP] [<user>...]\n", argv[0]);
    exit(1);
}


/* Some dummy routines that never get called, but need to exist to let
 * things link.  Most of these are called by newpart() which will never
 * be called because we always open participation files in snoop mode.
 */

void seek_item(int x) {}   /* DUMMY */
int next_item(int *a,int *b,int *c,time_t *d,time_t *e) {return 1;}  /*DUMMY*/
int in_sel(int i, char *s) {return 0;} /* DUMMY */
char *user_fname() {return "";} /* DUMMY */
