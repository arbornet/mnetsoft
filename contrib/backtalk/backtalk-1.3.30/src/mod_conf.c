/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Front-End of Backtalk BBS interface functions -- PicoSpanish edition.
 * The more commonly used functions are in picospan.c
 */

#include "backtalk.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <pwd.h>

#include "adm_conf.h"
#include "user.h"
#include "stack.h"
#include "sysdict.h"
#include "udb.h"
#include "str.h"
#include "ps_config.h"
#include "ps_part.h"
#include "ps_sum.h"
#include "ps_edit.h"
#include "baaf_core.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


extern int cf_maypost;

/* FUNC_HIDE_RESP:	- <flag> hide_resp <err>
 *
 *   Stack Inputs:
 *	<flag>		nonzero to hide, zero to unhide.
 *   Variable Inputs:
 *	"item"		Item number.
 *	"resp"		response to hide.
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"amfw"		Fairwitness flag
 *   Stack Outputs:
 *	<err>		0=success, 1=already hidden, 2=no such item or resp
 *
 * "Open_conf" should have been called.
 */

void func_hide_resp()
{
    int hide= pop_integer();

    if (!cf_maypost)
	die("Attempt to modify read-only conference");

    push_integer(hide_resp(idict(VAR_ITEM),idict(VAR_RESP),hide));
}


/* FUNC_ERASE_RESP:	- erase_resp <err>
 *
 *   Variable Inputs:
 *	"item"		Item number.
 *	"resp"		response to erase.
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"amfw"		Fairwitness flag
 *   Stack Outputs:
 *	<err>		0=success, 1=already erased, 2=no such item or resp
 *
 * "Open_conf" should have been called.
 */

void func_erase_resp()
{
    if (!cf_maypost)
	die("Attempt to modify read-only conference");

    push_integer(erase_resp(idict(VAR_ITEM),idict(VAR_RESP)));
}


/* FUNC_FREEZE_ITEM:	- <flag> freeze_item <err>
 *
 *   Stack Inputs:
 *	<flag>		nonzero to freeze, zero to thaw.
 *   Variable Inputs:
 *	"item"		Item number.
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"amfw"		Fairwitness flag
 *   Stack Outputs:
 *	<err>		0=success, 2=no such item, 3=no access, 1=already set
 *
 * "Open_conf" should have been called.
 */

void func_freeze_item()
{
    int freeze= pop_integer();

    if (cf_maypost)
	push_integer(item_mode(idict(VAR_ITEM), freeze ? IM_FREEZE : IM_THAW));
    else
	push_integer(3);
}


/* FUNC_RETIRE_ITEM:	- <flag> retire_item <err>
 *
 *   Stack Inputs:
 *	<flag>		nonzero to retire, zero to unretire.
 *   Variable Inputs:
 *	"item"		Item number.
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"amfw"		Fairwitness flag
 *   Stack Outputs:
 *	<err>		0=success, 2=no such item, 3=no access, 1=already set
 *
 * "Open_conf" should have been called.
 */

void func_retire_item()
{
    int retire= pop_integer();

    if (cf_maypost)
	push_integer(item_mode(idict(VAR_ITEM),
	    retire ? IM_RETIRE : IM_UNRETIRE));
    else
	push_integer(3);
}


/* FUNC_KILL_ITEM:	- kill_item <err>
 *
 *   Variable Inputs:
 *	"item"		Item number.
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"amfw"		Fairwitness flag
 *   Stack Outputs:
 *	<err>		0=success, 2=no such item, 3=no access
 *
 * "Open_conf" should have been called.
 */

void func_kill_item()
{
    if (cf_maypost)
	push_integer(kill_item(idict(VAR_ITEM)));
    else
	push_integer(3);
}


/* FUNC_RETITLE_ITEM:	<newtitle> retitle_item <err>
 *
 *   Variable Inputs:
 *	"item"		Item number.
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"amfw"		Fairwitness flag
 *   Stack Inputs:
 *      <newtitle>	New title for the item.
 *   Stack Outputs:
 *	<err>		0=success, 1=bad title, 2=no such item, 3=no access
 *
 * "Open_conf" should have been called.
 */

void func_retitle_item()
{
    char *title= pop_string();

    if (cf_maypost)
	push_integer(retitle_item(title));
    else
	push_integer(3);
    free(title);
}


/* FUNC_EDIT_RESP:	edit_resp <err>
 *
 *   Variable Inputs:
 *      "text"		New text.
 *      "texttype"	New Text Type.
 *      "alias"		Alias
 *      "parent"	Parent
 *	"resp"		Response number.
 *	"item"		Item number.
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"amfw"		Fairwitness flag
 *   Stack Inputs:
 *      none.
 *   Stack Outputs:
 *	<err>		0=success, 2=no such item/resp, 3=no access
 *
 * "Open_conf" should have been called.
 */

void func_edit_resp()
{
    if (cf_maypost)
	push_integer(edit_resp(TRUE,NULL,NULL));
    else
	push_integer(3);
}


/* FUNC_REATTACH:	<delattach> <addattach> reattach <err>
 *
 *   Variable Inputs:
 *	"resp"		Response number.
 *	"item"		Item number.
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"amfw"		Fairwitness flag
 *   Stack Inputs:
 *      <delattach>     Delete attachment with this handle.
 *      <addattach>	Add attachment with this handle.
 *   Stack Outputs:
 *	<err>		0=success, 2=no such item/resp, 3=no access,
 *		        4 no such attachment
 *
 * Delete, add or replace an attachment.  "Open_conf" should have been called.
 */

#if ATTACHMENTS
void func_reattach()
{
    char *addattach= pop_string();
    char *delattach= pop_string();
    int rc= 0;

    if (addattach[0] != '\0' || delattach[0] != '\0')
    {

	if (cf_maypost)
	{
	    rc= edit_resp(FALSE, delattach[0] != '\0' ? delattach : NULL,
				 addattach[0] != '\0' ? addattach : NULL);
	    if (rc == 0 && delattach[0] != '\0')
		del_attach(delattach);
	}
	else
	    rc= 3;
    }
    push_integer(rc);

    free(delattach);
    free(addattach);
}
#else
void func_reattach() { func_pop(); func_pop(); push_integer(4);}
#endif


/* FUNC_LINK_ITEM:	<conf> <item> link_item <newitem> 0
 *                      <conf> <item> link_item <err>
 *
 *   Stack Inputs:
 *	<conf>		Source conference
 *	<item>		Source item
 *   Variable Inputs:
 *	"conf"		Destination conference
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"amfw"		Fairwitness flag
 *   Stack Outputs:
 *      <newitem>	Destination item number
 *	<err>		0=success, 1= cannot access conference,
 *			2=no such item, 3=no access
 */

void func_link_item()
{
    int item= pop_integer();
    char *conf= pop_string();
    int rc,new_item;

    if (cf_maypost)
    {
	if ((rc= link_item(item,conf,&new_item)) == 0)
	    push_integer(new_item);
    }
    else
	rc= 3;
    free(conf);
    push_integer(rc);
}


/* KILLDIR - Remove a directory and all files below it. */

static void killdir(char *dir)
{
    pid_t cpid, wpid;

    if ((cpid= fork()) == 0)
    {
	execl("/bin/rm","rm","-rf",dir,(char *)NULL);
	exit(1);
    }

    while ((wpid= wait((int *)0)) != cpid && wpid != -1)
	;
}


/* DEL_PARTICIPATION - For all users, delete the named participation files.
 */

static void del_participation(char *pfname)
{
#ifdef UNIX_ACCOUNTS
    /* Not implemented yet for real unix accounts (because it is harder to
     * do right)
     */
#else
    char *login;
    struct passwd *pw;
    char buf[BFSZ];

    /* Loop through all users */
    for (login= firstusr(); login != NULL; login= nextusr())
    {
	if ((pw= getdbnam(login)) == NULL) continue;
	sprintf(buf,"%s/%s",pw->pw_dir,pfname);
	unlink(buf);
    }
#endif
}


/* FUNC_NEW_CONF:  <conf> <parentdir> <listit> newconf <error>
 * Create a new conference.  The conference will be stored in a directory
 * under <parentdir> with <conf> as its name.  If <listit> is true, it will get
 * an entry in the conflist with that name, so that it can be accessed by
 * that name.  If <parentdir> is (), it defaults to BBS_DIR.  On failure,
 * pushes an error string, () on success.
 */

void func_new_conf()
{
    char *err, *pfile;
    char *conf, *parent;
    int listit;
    char dir[BFSZ],pfname[BFSZ];

    if (!bdict(VAR_AMADM))
	die("You are not a conference administrator.");

    /* Get the arguments */

    listit=  pop_boolean();
    parent= pop_string();
    conf= pop_string();

    /* Set default parent directory if none given */
    if (parent[0] == '\0')
	{ free(parent); parent= strdup(sdict(VAR_BBSDIR)); }

    pfile= make_part_name(conf);

    if (err= check_conf(conf, parent, pfile))
    {
	push_string(err,TRUE);
	return;
    }

    sprintf(dir,"%.100s%.100s",parent,conf);
    make_confdir(dir, pfile, DFLT_FW, 0, NULL);
    free(pfile);

    if (listit) add_conflist(conf,dir);

    free(conf);
    free(parent);

    push_string("",TRUE);
}


/* FUNC_DEL_CONF: <conf> delconf -
 * Delete a conference.
 */

void func_del_conf()
{
    char *err, *pfile;
    char *conf;
    char dir[BFSZ],pfname[BFSZ];

    if (!bdict(VAR_AMADM))
	die("You are not a conference administrator.");

    /* Get the arguments */

    conf= pop_string();
    sprintf(dir,"%.100s%.100s",sdict(VAR_BBSDIR),conf);

    /* Delete all entries from conflist */
    del_conflist(dir);

    /* Get participation file name */
    readconfig(dir,pfname,NULL,NULL,NULL);

    /* Remove conference directory */
    killdir(dir);

    /* Nuke all user's participation files */
    del_participation(pfname);
    
    free(conf);
}
