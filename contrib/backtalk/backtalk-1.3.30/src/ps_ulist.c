/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <stdio.h>
#include <sys/stat.h>

#include "str.h"
#include "readfile.h"
#include "sysdict.h"
#include "udb.h"
#include "ps_ulist.h"
#include "stack.h"
#include "group.h"
#include "lock.h"
#include "partdir.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* IN_ULIST - check if a user is in the ulist for the current confernce.
 */

int in_ulist(char *id, char *file)
{
    char path[BFSZ+1];
    char bf[BFSZ+1];
    FILE *fp;
    int len;

    if (id == NULL || id[0] == '\0')
	return 0;
    len= strlen(id);

    if (*file == '/')
	strcpy(path,file);
    else
	sprintf(path,"%s/%s",sdict(VAR_CONFDIR),file);
    if ((fp= fopen(path,"r")) == NULL)
	return 0;
    lock_shared(fileno(fp),path);
    
    while (fgets(bf, BFSZ, fp) != NULL)
    {
	if ((bf[len] == '\n' || bf[len] == '\r') && !strncmp(bf,id,len))
	{
	    fclose(fp);
	    return 1;
	}
    }
    fclose(fp);
    unlock(path);
    return 0;
}


/* IN_GLIST - check if a user is in the glist for the current conference
 */

int in_glist(char *id, gid_t gid, char *file)
{
    char path[BFSZ+1];
    char bf[BFSZ+1];
    FILE *fp;
    int len, i, rc;
    int ngroups= 0, maxgroups= 5;
    char **groups;

    if (id == NULL || id[0] == '\0')
	return 0;
    if (gid == cfadm_group_id())
    	return 1;

    /* Initialize an array for group names allowed to log in */
    groups= (char **)malloc(maxgroups*sizeof(char *));
    groups[ngroups++]= strdup(cfadm_group_name());

    /* Read glist file into array */
    if (*file == '/')
	strcpy(path,file);
    else
	sprintf(path,"%s/%s",sdict(VAR_CONFDIR),file);
    if ((fp= fopen(path,"r")) == NULL)
	return 0;
    lock_shared(fileno(fp),path);
    
    while (fgets(bf, BFSZ, fp) != NULL)
    {
        len= strlen(bf);
	if (bf[0] != '#' && len > 1 && bf[len-1] == '\n')
	{
	    if (len > 2 && bf[len-2] == '\r') len--;
	    bf[len-1]= '\0';
	    if (ngroups >= maxgroups-1)
	    {
	    	ngroups*= 2;
		groups= (char **)realloc(groups, ngroups);
	    }
	    groups[ngroups++]= strdup(bf);
	}
    }
    fclose(fp);
    unlock(path);
    groups[ngroups]= NULL;

    /* Check if we are in any of those groups */
    rc= ingrouplist(groups,gid,id);

    /* Free memory */
    for (i= 0; groups[i] != NULL; i++)
    	free(groups[i]);
    free(groups);

    return rc;
}


/* CPASS_OK - Check if the user gave the correct conference password.
 * If the file is missing or empty, no password is correct.
 */

int cpass_ok(char *cpass)
{
    char bf[BFSZ+1];
    FILE *fp;
    int len= strlen(cpass);
    char *rc;

    sprintf(bf,"%s/secret",sdict(VAR_CONFDIR));
    if ((fp= fopen(bf,"r")) == NULL)
	    return 0;
    
    rc= fgets(bf, BFSZ, fp);
    fclose(fp);

    if (rc == NULL)
	 return 0;
    else
	return (bf[len] == '\n' || bf[len] == '\r') && !strncmp(bf,cpass,len);
}


#ifdef KEEP_ULIST

/* ADD_ULIST - add a user to the ulist, if he isn't already there.
 */

void add_ulist(char *id)
{
    char path[BFSZ+1];
    char bf[BFSZ+1];
    FILE *fp;
    int len;

    if (id == NULL || id[0] == '\0')
	return;
    len= strlen(id);

    sprintf(path,"%s/ulist",sdict(VAR_CONFDIR));
    if ((fp= fopen(path,"a+")) == NULL)
	return;
    lock_shared(fileno(fp),path);
    fseek(fp,0L,0);
    
    /* Check if it is already in the file */
    while (fgets(bf, BFSZ, fp) != NULL)
    {
	if (bf[len] == '\n' && !strncmp(bf,id,len))
	{
	    fclose(fp);
	    return;
	}
    }

    /* Append to the file */
    fseek(fp,0L,2);
    fprintf(fp,"%s\n",id);
    fclose(fp);
    unlock(path);
}


/* DEL_ULIST - remove a user from the ulist.
 */

void del_ulist(char *id)
{
    char path[BFSZ+1];
    FILE *fp;
    int len, nl;
    char *ulist, *u;

    if (id == NULL || id[0] == '\0')
	return;
    len= strlen(id);

    sprintf(path,"%s/ulist",sdict(VAR_CONFDIR));

    /* Load the whole file into memory */
    if ((ulist= read_file(path,TRUE)) == NULL)
    	return;

    if ((fp= fopen(path,"w")) == NULL)
	return;

    lock_exclusive(fileno(fp),path);

    /* Write out all except the user */
    nl= 1;
    for (u= ulist; *u != '\0'; u++)
    {
        if (nl && u[len] == '\n' && !strncmp(u,id,len))
            u+= len;
        else
        {
	    fputc(*u,fp);
	    nl= (*u == '\n');
        }
    }
    fclose(fp);
    unlock(path);
}

#endif /* KEEP_ULIST */

FILE *ppant_fp= NULL;
int ppant_state= -1;	/* -1 = not open, 0 = read first, 1 = read next */
#ifdef KEEP_ULIST
#define ppant_ulist 1
#else
int ppant_ulist;	/* true if we are using ulist for current scan */
#endif

/* CLOSE_PARTICIPANT
 *
 * End the participant scan, if in progress.
 */

void close_participant()
{
    if (ppant_fp == NULL) return;
    fclose(ppant_fp);
    ppant_fp= NULL;
    ppant_state= -1;
}


/* REWIND_PARTICIPANT
 *
 * This should be called before the first call to next_participant(), and
 * can be called again to rewind the participant list.
 */

void rewind_participant()
{
    char bf[BFSZ+1];

    close_participant();
    ppant_state= 0;
#ifndef KEEP_ULIST
    if ((ppant_ulist= bdict(VAR_USERLIST)) != 0)
#endif
    {
    	sprintf(bf,"%s/ulist",sdict(VAR_CONFDIR));
	ppant_fp= fopen(bf,"r");
	if (ppant_fp == NULL) ppant_state= -1;
#ifndef KEEP_ULIST
    	ppant_ulist= 1;
#endif
    }
}


/* READ_CFUSER - Read in the next possible participant.  For conferences with
 * a ulist, this is the next person on the ulist.  Otherwise it is the next
 * person in the user database.  Returns a pointer to the name in static
 * memory.  Return NULL if no more users found.
 */
 
char *read_cfuser()
{
    static char bf[MAX_LOGIN_LEN+21];
    char *usr= bf;

    if (ppant_state < 0) return NULL;

#ifndef KEEP_ULIST
    if (!ppant_ulist)
    {
	/* Get next user in user database */
	if (ppant_state == 0)
	{
	    usr= firstusr();
	    ppant_state= 1;
	}
	else
	    usr= nextusr();
	if (usr == NULL) return NULL;
    }
    else
#endif
    {
	/* Get next user in ulist */
	if (fgets(bf, MAX_LOGIN_LEN+20, ppant_fp) == NULL)
	{
	    close_participant();
	    return NULL;
	}
	*firstin(usr= firstout(bf," \t")," \n\r\t")= '\0';
    }
    return usr;
}


/* LAST_CF_ACCESS
 *
 * Return the date of the last access to the current conference by the
 * named user.  Returns 0 if no access is recorded.
 */

time_t last_cf_access(char *usr)
{
    char pfname[BFSZ];
    struct stat st;
    char *dir;

#ifdef PART_DIR
    bbspartpath(pfname,usr,sdict(VAR_PARTICIP));
#else
    if ((dir= gethomedir(usr)) == NULL) return 0;
    homepartpath(pfname,dir,sdict(VAR_PARTICIP));
#endif
    if (stat(pfname,&st))
    	return 0;
    else
	return st.st_mtime;

}

/* NEXT_PARTICIPANT
 *
 * Return the user name of the next participant in the conference.
 * Return NULL if htere is none.  Rewind_participant() should be called
 * before the first call to this.  Returns a pointer to the user name in
 * static storage, and the last access time.
 *
 * Ideally we would delete any participants we find in ulist files that don't
 * exist, but that's actually pretty complex to do right.
 */

char *next_participant(time_t *last)
{
    char *usr;

    for (;;)
    {
	/* Get next possible participant */
	if ((usr= read_cfuser()) == NULL)
	    return NULL;

	/* Check if user is a participant */
	if ((*last= last_cf_access(usr)) > 0) return usr;
    }
}


/* SEEK_PARTICIPANT
 *
 * Skip through the participant list until we find the named particpant.
 * The next call to next_participant will give his successor.
 */

void seek_participant(char *whom)
{
    char bf[MAX_LOGIN_LEN+21];
    char *usr= bf;

    if (ppant_state < 0) return;

    for (;;)
    {
	/* Get next possible participant */
	if ((usr= read_cfuser()) == NULL)
	    return;

	if (!strcmp(whom,usr))
	    return;
    }
}


/* FUNC_CHECK_PARTIC
 * <user> check_partic <lastaccess>
 * Check if a user is a participant in this conference.  If he is, push the
 * lastaccess time.  If not, push 0.
 */

void func_check_partic()
{
    char *usr= pop_string();

    push_time(last_cf_access(usr));
}


/* FUNC_FIRST_PARTIC
 * <start> first_partic <lastaccess> <user> 0
 * <start> first_partic 1
 *
 * Start a scan through the participants of this conference.  If start is
 * not the empty string, it is the name of a user to start *after*.
 */

void func_first_partic()
{
    char *seek= pop_string();
    char *usr;
    time_t tm;

    if (!bdict(VAR_MAYREAD))
    {
    	push_integer(1);
    	return;
    }

    rewind_participant();

    if (seek[0] != '\0')
    	seek_participant(seek);
    free(seek);

    usr= next_participant(&tm);
    if (usr == NULL)
    	push_integer(1);
    else
    {
	push_time(tm);
    	push_string(usr, TRUE);
    	push_integer(0);
    }
}


/* FUNC_NEXT_PARTIC
 * - next_partic <lastaccess> <user> 0
 * - next_partic 1
 */

void func_next_partic()
{
    char *usr;
    time_t tm;

    if (!bdict(VAR_MAYREAD))
    {
    	push_integer(1);
    	return;
    }

    usr= next_participant(&tm);
    if (usr == NULL)
    	push_integer(1);
    else
    {
	push_time(tm);
    	push_string(usr, TRUE);
    	push_integer(0);
    }
}
