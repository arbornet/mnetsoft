/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Front-End of Backtalk BBS interface functions -- PicoSpanish edition.
 * The more rarely used functions are now in mod_conf.c
 */

#include "backtalk.h"
#include "sysdict.h"
#include "conf.h"
#include "user.h"
#include "stack.h"
#include "str.h"
#include "file.h"
#include "dict.h"
#include "sel.h"
#include "ps_acl.h"
#include "ps_conflist.h"
#include "ps_config.h"
#include "ps_item.h"
#include "ps_part.h"
#include "ps_post.h"
#include "ps_cflist.h"
#include "ps_sum.h"
#include "ps_ulist.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int count_new(int *brandnew, int *newresp, int *unseen, int *total, int test);


/* FUNC_NEXT_CONF:	<csel> next_conf <csel> <conf>
 *
 *   Stack Inputs:
 *	<csel>		- Conference selector (@<number> or list of names)
 *   Variable Inputs:
 *	none
 *    Stack Outputs:
 *	<conf>		- Next conference name - null if none.
 *	<csel>		- Updated conference selector ("conf" deleted)
 *    Variable Ouputs:
 *	none
 *
 * This makes no attempt to determine if the named conference exists or not.
 */

void func_next_conf()
{
    char *csel= pop_string();

    if (csel[0] == '@')
    {
	char *cf;
	int i;

	/* Get next line from a .cflist file */
	if ((cf= nth_conf(i= atoi(csel+1))) == NULL)
	{
	    push_string("",TRUE);	/* <csel> */
	    push_string("",TRUE);	/* <conf> */
	    return;
	}
	else
	{
	    char bf[20];
	    sprintf(bf,"@%d",i+1);
	    push_string(bf,TRUE);	/* <csel> */
	    push_string(cf,FALSE);	/* <conf> */
	}
    }
    else
    {
	/* Get next name from list in csel */
	char *e= strchr(csel,',');
	if (e == NULL)
	{
		push_string("",TRUE);	/* <csel> */
		push_string(csel,TRUE);	/* <conf> */
	}
	else
	{
		*e= '\0';
		push_string(e+1,TRUE);	/* <csel> */
		push_string(csel,TRUE);	/* <conf> */
	}
    }
    free(csel);
}


/* FUNC_OPEN_CONF	-  open_conf <err>
 *
 *   Stack Inputs:
 *	 none
 *   Variable Inputs:
 *	 "conf"         - Name of current conference.
 *	 "id"		- ID of the current user.
 *	 "cpass"	- Conference password.
 *    Stack Outputs:
 *        <err>         - 0 normally;
 *			  1 if no such conf;
 *    Variable Ouputs:
 *        "amfw"	- Is the current user a fairwitness?
 *        "alias"	- The current user's alias in this conference.
 *        "confdir"	- Full pathname of conference directory.
 *        "conftitle"   - Title of conference.
 *	  "fwlist"	- a comma separated list of fairwitnesses.
 *	  "mayread"	- Current user may read this conference.
 *	  "maypost"	- Current user may post to this conference.
 *	  "maxitem"	- Number of largest item.
 *	  "particip"	- Name of participation file.
 *	  "fishbowl"	- Conference is readonly to people not in ulist.
 *	  "userlist"	- Conference has a user list.
 *	  "secret"	- 0 if conference has no password,
 *                        1 if conference has password and it was given
 *                        2 if conference has password and it was not given
 *	  "newbie"	- True if conference was joined for the first time
 *
 * If the conference is already open, this is a no-op.
 *
 * Most of the other commands implicitly operate on the currently open
 * conference.
 */

#define CONFNAMELEN  50
char curr_conf[CONFNAMELEN+1]= "";	/* Name of currently open conference */
int private_item;	/* True if items in conf should not be readable */
int cf_mayresp;		/* "mayresp" setting for current conference */
int cf_maypost;		/* "maypost" setting for current conference */

void do_open_conf(int snoop)
{
    char *conf= sdict(VAR_CONF);
    int mode, noenterflag;
    int mayread, have_acl= 0;
    int new;
    char *dir, *title;
    char fw[BFSZ];
    char pf[BFSZ];

    if (conf == NULL || conf[0] == '\0')
    	die("conf system variable is not set");

    /* Check if conference is already open */
    if (strcasecmp(conf,curr_conf))
    {
	/* Find the configuration file */
	if ((dir= cfpath(conf)) == NULL)
	{
	    push_integer(1);
	    return;
	}
	/* Close any previous conference */
	func_close_conf();

	strncpy(curr_conf,conf,CONFNAMELEN);
	curr_conf[CONFNAMELEN]= '\0';
	sets(VAR_CONFDIR,dir,SM_COPY);

	readconfig(dir, pf, fw, &mode, &title);
	sets(VAR_PARTICIP,pf,SM_COPY);
	sets(VAR_FWLIST,fw,SM_COPY);
	seti(VAR_AMFW, bdict(VAR_AMADM) || inlist(sdict(VAR_ID),fw," ,"));
	if (title == NULL)
	{
	    /* If no conf title in config, use capitalized version of conf */
	    title= strdup(conf);
	    capfirst(title);
	    sets(VAR_CONFTITLE, title, SM_FREE);
	}
	else
            sets(VAR_CONFTITLE, title, SM_COPY);

	/* Open sum file and set VAR_MAXITEM */
	seti(VAR_MAXITEM,last_no());

	/* Open participation file and set VAR_ALIAS */
	new= openpart(snoop);

	seti(VAR_NEWBIE, new);

#ifndef YAPP_COMPAT
	if (mode < 0)	/* check ACL file only if mode was 'acl' */
#endif
	    have_acl= read_acl(dir, &mayread, &cf_mayresp, &cf_maypost);

	if (have_acl)
	{
	    seti(VAR_MAYREAD,mayread);
	    seti(VAR_MAYRESP,cf_mayresp);
	    seti(VAR_MAYPOST,cf_maypost);
	}
	else if (mode < 0)
	    die("config file specifies 'acl' but no acl file found");
	else
	{
	    /* Check modes -- This needs to be fixed up to understand ulists
	     * and passwords.  Currently allow people into readable
	     * conferences only.
	     *   mode==0 -> may join
	     *   mode==4 -> may join if in ulist
	     *   mode==5 -> may join with password
	     *   mode==6 -> may join with password if in ulist
	     *   mode==8 -> may join
	     * If the mode is non-zero, new items should be created unreadable.
	     * Yapp supports read-only conferences, which add 16 to the above
	     * modes.
	     */
	    seti(VAR_FISHBOWL, (mode&0x10) != 0);
	    noenterflag= ((mode&0x100) != 0);
	    mode&= 0x0F;
	    seti(VAR_USERLIST, (mode == 4 || mode == 6));
	    seti(VAR_GROUPLIST, (mode == 4 || mode == 6));
	    seti(VAR_SECRET, (mode == 5 || mode == 6));
	    private_item= (mode != 0);

	    cf_mayresp= (idict(VAR_MAXITEM) > 0 || bdict(VAR_AMFW));
	    mayread= 1;

	    if (bdict(VAR_USERLIST) && !bdict(VAR_AMFW) &&
		!in_ulist(sdict(VAR_ID),"ulist") &&
		!in_glist(sdict(VAR_ID),idict(VAR_GID),"glist"))
	    {
		    /* Not in ulist or glist */
		    mayread= bdict(VAR_FISHBOWL);
		    cf_mayresp= FALSE;
	    }
	    if (bdict(VAR_SECRET) && !bdict(VAR_AMFW) &&
		     !cpass_ok(sdict(VAR_CPASS)))
	    {
		    /* Don't know password */
		    seti(VAR_SECRET, 2);
		    if (mayread) mayread= bdict(VAR_FISHBOWL);
		    cf_mayresp= FALSE;
	    }
	    seti(VAR_MAYREAD,mayread);

	    seti(VAR_MAYRESP,cf_mayresp && sdict(VAR_ID)[0] != '\0');
	    cf_maypost= noenterflag ? bdict(VAR_AMFW) : cf_mayresp;
	    seti(VAR_MAYPOST,cf_maypost && sdict(VAR_ID)[0] != '\0');
	}

	/* Set anonymity flag */
	switch (idict(VAR_ANONYMITY))
	{
	default:
	case 0:
	    seti(VAR_MAYSEEALIAS, 1);
	    break;
	case 1:
	    seti(VAR_MAYSEEALIAS,
	    	(idict(VAR_UID) >= 0 && sdict(VAR_ID)[0] != '\0'));
	    break;
	case 2:
	    seti(VAR_MAYSEEALIAS, bdict(VAR_AMFW));
	    break;
	case 3:
	    seti(VAR_MAYSEEALIAS, bdict(VAR_AMADM));
	    break;
	}

#ifdef KEEP_ULIST
	/* If we are joining a non-ulist conference for first time,
	 * add us to ulist
	 */
	if (new && !bdict(VAR_USERLIST))
		add_ulist(sdict(VAR_ID));
#endif
    }

    push_integer(0);
}

void func_open_conf()
{
    do_open_conf(FALSE);
}

/* SNOOP_CONF  - snoop_conf <err>
 * This is just like open_conf, except we don't create or edit participation
 * files.
 */

void func_snoop_conf()
{
    do_open_conf(TRUE);
}


/* FUNC_CLOSE_CONF	-  close_conf -
 *
 *   Stack Inputs:
 *	 none
 *   Variable Inputs:
 *	 none
 *    Stack Outputs:
 *	 none
 *    Variable Ouputs:
 *	 none
 *
 * This closes the currently open conference, if one is open, writing out
 * the participation file and what-not.  It is automatically called before
 * termination, so it is not necessary to ever use this operator.
 */

void func_close_conf()
{
    if (curr_conf[0] != '\0')
    {
	closeitem();
	closesum();
	closepart();
	curr_conf[0]= '\0';
    }
}


/* FUNC_RESIGN_CONF      - resign_conf -
 *
 *   Stack Inputs:
 *	 none
 *   Variable Inputs:
 *	 none
 *    Stack Outputs:
 *	 none
 *    Variable Ouputs:
 *	 none
 *
 * This resigns the user from the currently open conference, discarding all
 * memory of ever having been in it.  It also closes the conference.
 */
 
void func_resign_conf()
{
    if (curr_conf[0] == '\0')
	die("No conference currently open");

#ifdef KEEP_ULIST
    /* If this is NOT a ulisted conference, delete us from the user list */
    if (!bdict(VAR_USERLIST))
	    del_ulist(sdict(VAR_ID));
#endif
    closeitem();
    closesum();
    removepart();
}


/* FUNC_LAST_IN_CONF      - find date of last visit to conference -
 *
 *   Stack Inputs:
 *	 none
 *   Variable Inputs:
 *	 none
 *    Stack Outputs:
 *	 <time>        Time we last modified participation file for currently
 *                     open conference, or 0 if we never have.
 *    Variable Ouputs:
 *	 none
 */
 
void func_last_in_conf()
{
    if (curr_conf[0] == '\0')
	die("No conference currently open");

    push_time(partdate());
}


/* COUNT_NEW - Find the number of new items in the current conference.
 * If test is true, stop after we have found one new one.  Arguments
 * returned (if addresses are not NULL) are number of brandnew items,
 * number of items with new responses, and total number of items (including
 * forgotten ones).  The function returns the sum of the first two numbers.
 */

int count_new(int *brandnew, int *newresp, int *unseen, int *total, int test)
{
    int item, nresp, flags, forgotten, rresp;
    time_t idate, rdate, ldate;
    int n_brandnew= 0, n_newresp= 0, n_unseen= 0;

    if (total != NULL) *total= 0;

    for (seek_item(1); !next_item(&item,&nresp,&flags,&idate,&rdate); )
    {
        if (total != NULL) (*total)++;

	last_read(item, &rresp, &forgotten, &ldate);

	/* Skip forgotten or retired items */
	if (((forgotten || (flags&SF_RETIRED)) &&
	     idict(VAR_SHOWFORGOTTEN) == 0 ) ||
	    (!forgotten && !(flags&SF_RETIRED) &&
	     idict(VAR_SHOWFORGOTTEN) == 2))
		continue;

	if (nresp-1 <= rresp)
	    continue;

	if (rdate <= ldate)
	{
	    n_unseen++;
	    continue;
	}

	if (rresp == -1)
	    n_brandnew++;
	else
	    n_newresp++;

	if (test) break;
    }

    if (brandnew != NULL) *brandnew= n_brandnew;
    if (newresp != NULL) *newresp= n_newresp;
    if (unseen != NULL) *unseen= n_unseen;

    return (n_brandnew + n_newresp);
}


/* FUNC_CONF_NEW:	- conf_new <total> <brandnew> <newresp> <unseen>
 *
 * Find the number of brandnew and newresp items in the current conference.
 */

void func_conf_new()
{
    int brandnew, newresp, unseen, total;

    (void) count_new(&brandnew, &newresp, &unseen, &total, FALSE);
    push_integer(total);
    push_integer(brandnew);
    push_integer(newresp);
    push_integer(unseen);
}


/* FUNC_NEXT_ITEM:	<is> next_item <is> <nrs> <item> <err>
 *
 *   Stack Inputs:
 *	 <is>		- Item selector string
 *   Variable Inputs:
 *	 "rsel" 	- Response selector (might not be numeric).
 *	 "noskip" 	- Don't skip items with no selected responses.
 *	 "showforgotten"- Don't skip forgotten and retired items.
 *    Stack Outputs:
 *        <is>          - Updated item selector
 *        <nrs>         - Numeric response selector
 *	  <item>	- number of next item
 *        <err>         - 0 normally
 *			- 1 if no such item (nothing else on stack).
 *			- 2 if bad item selector (nothing else on stack).
 *    Variable Ouputs:
 *	maxitem		- may be corrected if we hit end of items early.
 *
 * "rsel" may be a keyword like "new", but the pushed <nrs> will always be
 * a numeric one.
 * 
 * If "rsel" is new and <nrs> would be {}, then the item is skipped
 * internally.
 */

/* Store description of item found, for possible use by this_item */
int the_item, the_nresp, the_flags, the_resp_read, the_forgotten;
time_t the_item_date, the_resp_date, the_read_date;

void func_next_item()
{
    int last_resp, readnew;
    int rc;

    if (curr_conf[0] == '\0')
	die("No conference currently open");

    if (!bdict(VAR_MAYREAD))
	die("Attempt to read from private conference");

    /* Scan for item */
    for (;;)
    {
	/* Get item number from item selector, and update item selector */
	if ((the_item= next_int(-idict(VAR_MAXITEM))) < 0)
		/* End of item list or bad selector syntax */
		break;

	/* Ignore item number zero - it never exists */
	if (the_item == 0) continue;

	/* Fetch sum file entry for that item */
	if ((rc= get_sum(the_item, &the_nresp, &the_flags,
				   &the_item_date, &the_resp_date))==1)
	    /* Skip killed items */
	    continue;
	else if (rc == 2)
	{
	    /* Passed last item */
	    seti(VAR_MAXITEM,last_no());
	    the_item= -1;
	    break;
	}

	last_resp= the_nresp-1;

	/* Fetch participation file entry for that item */
	last_read(the_item, &the_resp_read, &the_forgotten, &the_read_date);

	/* Skip forgotten or retired items */
	if (!bdict(VAR_NOSKIP) &&
	    (((the_forgotten || (the_flags&SF_RETIRED)) &&
	      idict(VAR_SHOWFORGOTTEN) == 0 ) ||
	     (!the_forgotten && !(the_flags&SF_RETIRED) &&
	      idict(VAR_SHOWFORGOTTEN) == 2)))
		continue;

	/* Convert response selector to numeric form and push it */
	if (sdict(VAR_RSEL)[0] == '-')
	{
	    int backup= atoi(sdict(VAR_RSEL)+1);
	    if (backup == 0)
		push_string("",TRUE);
	    else if (backup > the_nresp)
		push_string("0-$",TRUE);
	    else
	    {
		char bf[BFSZ];
		sprintf(bf,"%d-$",the_nresp - backup);
		push_string(bf,TRUE);
	    }
	}
	else if ((readnew= !strcasecmp(sdict(VAR_RSEL),"new")) ||
		 !strcasecmp(sdict(VAR_RSEL),"unseen"))
	{
	    if (bdict(VAR_NOSKIP) ||
		(last_resp > the_resp_read &&
		 (!readnew || the_resp_date > the_read_date)))
	    {
		char bf[BFSZ];
		sprintf(bf,"%d-$",the_resp_read+1);
		push_string(bf,TRUE);
	    }
	    else
		/* No new/unseen responses - goto next item */
		continue;
	}
	else if (!strcasecmp(sdict(VAR_RSEL),"since"))
	{
	    if (bdict(VAR_NOSKIP) || (tdict(VAR_SINCE) <= the_resp_date))
	    {
		int old_item= idict(VAR_ITEM);
	        int resp= 0;
	        seti(VAR_ITEM,the_item);
		itemhead();
		while (tdict(VAR_DATE) < tdict(VAR_SINCE))
		{
		    resp++;
		    if (resphead(resp)) {resp= -1; break;}
		}
	        seti(VAR_ITEM,old_item);
		if (resp == -1)
		{
		    /* No responses actually found - which is weird */
		    if (bdict(VAR_NOSKIP))
			push_string("",TRUE);
		    else
		        continue;
		}
		else
		{
		    char bf[BFSZ];
		    sprintf(bf,"%d-$",resp);
		    push_string(bf,TRUE);
		}
	    }
	    else
		/* Nothing new since specified date - goto next item */
		continue;
	}
	else if (!strcasecmp(sdict(VAR_RSEL),"all"))
	    push_string("0-$",TRUE);
	else
	    push_string(sdict(VAR_RSEL),TRUE);
    
	/* Push found item number */
	push_integer(the_item);

	/* Push success code */
	push_integer(0);
	return;
    }

    /* No more items or bad syntax */
    func_pop();	/* Discard item selector from stack */
    push_integer(-the_item);
}


/* FUNC_THIS_ITEM:	<rsel> <item> this_item <nrsel> <err>
 *
 *   Stack Inputs:
 *	 <item>		- Number of current item
 *	 <rsel>		- Response selector (might not be numeric).
 *   Variable Inputs:
 *	 none
 *    Stack Outputs:
 *        <nrsel>	 - Numeric response selector
 *        <err>          - 0 normally
 *			 - 1 if no such item (nothing else on stack).
 *			 - 2 item exists, but no read acces to it.
 *    Variable Ouputs:
 *        "item"	 - item number - same one off stack.
 *        "maxresp"	 - Last response number
 *        "maxread"	 - Last response number previously read
 *        "linkdate"	 - Date item was linked into conference
 *        "lastdate"	 - Date of last response to item.
 *        "readdate"	 - Date we last read the item.
 *        "forgotten"	 - Is this item forgotten?
 *        "frozen"	 - Is this item frozen?
 *        "retired"	 - Is this item retired?
 *        "linked"	 - Is this item linked?
 *
 * This converts a non-numeric response selector to a numeric one that can be
 * used with "next_resp", and loads information from the participation file
 * and sum file entries for the item.  Thus is should be called before we
 * start reading responses of an item.  It will often be called immediately
 * after a "next_item" call.
 *
 * In some implementations, some of these variables may not be set until you
 * do a read_item.
 */

void func_this_item()
{
    int last_resp;
    char bf[20];
    int item= pop_integer();
    char *rsel= pop_string();

    if (!bdict(VAR_MAYREAD))
    {
	push_integer(2);
	free(rsel);
	return;
    }

    if (item < 1) die("Item number %d illegal",item);
    if (item != the_item)
    {
	if (get_sum(item, &the_nresp, &the_flags, &the_item_date,
			  &the_resp_date))
	{
	    push_integer(1);
	    free(rsel);
	    return;
	}
	last_read(item, &the_resp_read, &the_forgotten, &the_read_date);
    }

    seti(VAR_ITEM,item);
    last_resp= the_nresp-1;

    if (rsel[0] == '-')
    {
	int backup= atoi(rsel+1);
	if (backup == 0)
	    push_string("",TRUE);
        else if (backup > the_nresp)
	    push_string("0-$",TRUE);
	else
	{
	    sprintf(bf,"%d-$",the_nresp - backup);
	    push_string(bf,TRUE);
	}
    }
    else if (!strcmp(rsel,"new"))
    {
	if (last_resp > the_resp_read && the_resp_date > the_read_date)
	{
	    sprintf(bf,"%d-$",the_resp_read+1);
	    push_string(bf,TRUE);
	}
	else
	    push_string("",TRUE);
    }
    else if (!strcmp(rsel,"since"))
    {
	if (tdict(VAR_SINCE) <= the_resp_date)
	{
	    int resp= 0;
	    itemhead();
	    while (tdict(VAR_DATE) < tdict(VAR_SINCE))
	    {
		resp++;
		if (resphead(resp)) {resp= -1; break;}
	    }
	    if (resp == -1)
		push_string("",TRUE);
	    else
	    {
		char bf[BFSZ];
		sprintf(bf,"%d-$",resp);
		push_string(bf,TRUE);
	    }
	}
	else
	    push_string("",TRUE);
    }
    else if (!strcmp(rsel,"all"))
	push_string("0-$",TRUE);
    else
	push_string(rsel,TRUE);
    
    push_integer(0);

    seti(VAR_FORGOTTEN,the_forgotten);
    seti(VAR_MAXREAD,(the_read_date<the_resp_date ? the_resp_read : last_resp));
    seti(VAR_MAXRESP,last_resp);
    sett(VAR_LINKDATE,the_item_date);
    sett(VAR_LASTDATE,the_resp_date);
    sett(VAR_READDATE,the_read_date);
    seti(VAR_RETIRED,(the_flags&SF_RETIRED) != 0);
    seti(VAR_LINKED,(the_flags&SF_LINKED) != 0);
    seti(VAR_FROZEN,(the_flags&SF_FROZEN) != 0);
    if (cf_mayresp && sdict(VAR_ID)[0] != '\0')
	seti(VAR_MAYRESP, !bdict(VAR_FROZEN));

    free(rsel);
}


/* FUNC_READ_ITEM:      <nrs>  read_item  <nrs> <err>
 *
 *   Stack Inputs:
 *      <nrs>           - numeric response selector
 *   Variable Inputs:
 *      "item"		- item number
 *   Stack Outputs:
 *      <nrs>           - Possibly Updated numeric response selector(*).
 *      <err>            - 0 normally, 1 if no such item, 2 if bad nrs.
 *   Variable Outputs:
 *      "title"         - item title
 *      "resp"          - 0 or -1 (see Note)
 *      "authorname"    - Full Name of author
 *      "authorid"      - ID of author (just login or full email address)
 *      "authoruid"     - UID of author
 *      "attachments"   - List of attachments
 *      "date"          - Integer date value
 *      "hidden"        - Nonzero if response 0 is hidden
 *      "erased"        - nonzero if response 0 is scribbled
 *	"frozen"	- may be corrected if sum file had it wrong.
 *	"linked"	- may be corrected if sum file had it wrong.
 *	"retired"	- may be corrected if sum file had it wrong.
 *	"maypost"	- may be corrected if sum file had "frozen" wrong.
 *	"mayerase"	- Could the user erase the response 0 text?
 *	"mayhide"	- Could the user hide or show the response 0 text?
 *      "mayfreeze"	- Could the user freeze or thaw this item?
 *      "mayretire"	- Could the user retire or unretire this item?
 *      "maykill"	- Could the user kill this item?
 *
 * This reads in the header of an item.  A call to "next_item" or "this_item"
 * should have been done first.
 * 
 * (*) If the <nrs> starts with response 0, then "resp" is set to 0 and
 *     the 0 is consumed off the <nrs> that is pushed back on the stack.
 *
 *     If the <nrs> does not start with 0, then "resp" is set to -1 and
 *     the <nrs> is not changed.  Following this with a call to
 *     "read_text" may not be legal in this case.
 */

void func_read_item()
{
    char *nrs= peek_string();
    int amauthor;

    if (!bdict(VAR_MAYREAD))
	die("Attempt to read from private conference");
    if (nrs[0] == '0' && (nrs[1]=='\0' || nrs[1] == '-' || nrs[1] == ','))
    {
	/* Pull the first element out of the nrs */
	if (next_int(-1) == -2)
	{
	    free(pop_string());
	    push_integer(2);
	    return;
	}
	seti(VAR_RESP,0);
    }
    else
	seti(VAR_RESP,-1);

    if (itemhead())
    {
	func_pop(); 	/* discard nrs */
	push_integer(1);
    }
    else
	push_integer(0);

    if (cf_mayresp && sdict(VAR_ID)[0] != '\0')
	seti(VAR_MAYRESP, !bdict(VAR_FROZEN));

    item_powers();
}


/* FUNC_NEXT_RESP:     <nrs>  next_resp  <nrs> <resp> <err>
 *
 *   Stack Inputs:
 *	<nrs>  - Numeric Response Selector which specifies responses to be
 *		 scanned.
 *   Stack Outputs:
 *	<nrs>  - Updated Numeric Response Selector.  Same as before, but
 *		 with first response deleted.
 *	<resp> - Next response number.  An integer.
 *	<err>  - 0 normally
 *		 1 no more responses (nothing else on stack)
 *		 2 syntax error in selector (nothing else on stack)
 *   Variable Ouputs:
 *	none
 */

void func_next_resp()
{
    int nr;

    if ((nr= next_int(idict(VAR_MAXRESP))) < 0)
    {
	func_pop();
	push_integer(-nr);
	return;
    }
    push_integer(nr);
    push_integer(0);
}


/* FUNC_READ_RESP:     <resp>  read_resp  <err>
 *
 *   Stack Inputs:
 *	<resp> - Response number to read.  An integer.
 *   Variable Inputs:
 *	"item" - Number of current item.
 *   Stack Outputs:
 *	<err>  - 0 normally
 *		 1 no such responses
 *   Variable Outputs:
 *	"resp"		- response number (same one from stack)
 *	"authorname"	- Full Name of author
 *	"authorid"	- ID of author (just login or full email address)
 *	"authoruid"	- UID of author
 *	"date"		- Integer date value
 *	"hidden"	- Nonzero if response is hidden
 *	"erased"	- nonzero if response is scribbled
 *	"mayerase"	- Could the user erase the current response text?
 *	"mayhide"	- Could the user hide or show the current response text?
 *      "texttype"      - "text/plain" or "text/html".
 *	  
 * This is implemented so that it works efficiently when successive
 * calls read successive responses in increasing sequence (likely with
 * read_text calls interleaved).  However it should work correctly
 * to read responses in any sequence.
 */

void func_read_resp()
{
    int resp= pop_integer();

    if (!bdict(VAR_MAYREAD))
	die("Attempt to read from private conference");
    if (resp < 0)
	die("Response number may not be negative");
    seti(VAR_RESP,resp);
    push_integer(resp == 0 ? itemhead() : resphead(resp));
#ifdef PARENT_POINTERS
    /* No forward parent pointers */
    if (idict(VAR_PARENTRESP) >= resp) seti(VAR_PARENTRESP,0);
#endif
    resp_powers();
}


/* FUNC_READ_TEXT:	<format> read_text <text>
 *
 *   Stack Inputs:
 *	<format> - 0 = return plain text version
 *                 1 = return HTML version
 *                 2 = return best raw version
 *   Variable Inputs:
 *	<none>
 *   Stack Ouputs:
 *      <text> - Text of current response
 *
 * This returns the text corresponding to the last "read_resp" or
 * "read_item" command.  In some implementations, those commands may
 * already store the text, in which case this just retrieves it.
 *
 * This also marks all response of the current item up through the
 * one being fetched as "seen" unless the "blindfold" variable is true.
 */

void func_read_text()
{
    int format= pop_integer();

    if (format < 0 || format > 2)
	die("Unknown response format code %d",format);
    if (!bdict(VAR_MAYREAD))
	die("Attempt to read to private conference");
    push_string(resptext(format),FALSE);
}


/* FUNC_MARK_UNSEEN:	<resp> <item> mark_unseen <err>
 *
 *   Stack Inputs:
 *	<resp>  - response
 *	<item>	- item
 *   Stack Outputs:
 *	<err>	- 0 if successful, 1 if no such item, 2 if no such response
 *
 * This marks the given response, and all successors, unseen.  It operates
 * on the currently open conference.
 */

void func_mark_unseen()
{
    int item= pop_integer();
    int resp= pop_integer();
    int nresp;

    if (resp < 0)
	die("illegal response number %d",resp);
    if (get_sum(item, &nresp, NULL, NULL, NULL))
	push_integer(1);
    else if (--resp >= nresp)
	push_integer(2);
    else
    {
	update_item(item, resp, 1L, TRUE);
	push_integer(0);
    }
}


/* FUNC_FORGET_ITEM:	<item> forget_item <err>
 *
 *   Stack Inputs:
 *	<item>  - item number to forget
 *   Stack Outputs:
 *	<err>   - 0 if successful, 1 if item does not exist.
 */

void func_forget_item()
{
    int item= pop_integer();

    if (item < 1)
	die("illegal item number %d",item);
    if (get_sum(item, NULL, NULL, NULL, NULL))
	push_integer(1);
    else
    {
	update_item(item, -2, time(0L), FALSE);
	push_integer(0);
    }
}


/* FUNC_REMEMBER_ITEM:	<item> remember_item <err>
 *
 *   Stack Inputs:
 *	<item>  - item number to remember
 *   Stack Outputs:
 *	<err>   - 0 if successful, 1 if item does not exist.
 */

void func_remember_item()
{
    int item= pop_integer();

    if (item < 1)
	die("illegal item number %d",item);

    if (get_sum(item, NULL, NULL, NULL, NULL))
	push_integer(1);
    else
    {
	update_item(item, -4, time(0L), FALSE);
	push_integer(0);
    }
}


/* FUNC_POST_RESP:	- post_resp <resp_no>
 *
 *   Stack Inputs:
 *	none:
 *   Variable Inputs:
 *	"text"		Message to post.
 *	"item"		Item to post to.
 *	"alias"		Full name to place on response
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"hidden"	Response should be hidden.
 *	"erased"	Response should be erased (this would be weird).
 *      "texttype"      "text/html" or "text/plain".
 *   Stack Outputs:
 *	<resp_no>	Number of posted response, or 0 on failure
 *
 * "Open_conf" should have been called.
 */

void func_post_resp()
{
    if (!cf_mayresp)
	die("Attempt to respond to read-only item");
    
    push_integer(post_resp());
}


/* FUNC_POST_ITEM:	- post_item -
 *
 *   Stack Inputs:
 *	none
 *   Variable Inputs:
 *	"title"		Message to post.
 *	"text"		Message to post.
 *	"alias"		Full name to place on response
 *	"id"		User's login id
 *	"uid"		User's uid
 *	"hidden"	Item text should be hidden (weird)
 *	"erased"	Item text should be erased (even weirder).
 *      "texttype"      "text/html" or "text/plain".
 *   Variable Outputs:
 *	"item"		Item created
 *   Stack Outputs:
 *	none
 *
 * "Open_conf" should have been called.
 */

void func_post_item()
{
    if (!cf_maypost)
	die("Attempt to post to read-only conference");
	
    post_item();
}


/* FUNC_CONF_ALIAS	<name> conf_alias -
 *
 * Change the user's fullname in the currently open conference.
 * Open_conf must have been called.
 */

void func_conf_alias()
{
    set_conf_alias(pop_string());
}


/* FUNC_CONF_DIR	<name> conf_dir <directory>
 *
 * Given a conference name, return its directory path.  Note that open_conf
 * does the same thing and more, so this is only useful if you want to know
 * the directory path of a conference you probably don't intend to open.  This
 * is mainly happens when you want a unique identifier for a conference, which
 * directory names are, but conference names are not.  Pushs an empty string
 * if the conference does not exist.
 */

void func_conf_dir()
{
    char *name= pop_string();
    char *dir= cfpath(name);
    push_string(dir == NULL ? "" : dir,TRUE);
    free(name);
}


/* FUNC_CACHE_CONFLIST	<flag> cache_conflist -
 *
 * Enable or disable in memory caching of the conflist file.  This starts off
 * disabled.  If you plan to be doing lots of open_conf or conf_dir calls,
 * then enabling this will improve performance.
 */

void func_cache_conflist()
{
    cache_conflist(pop_boolean());
}


/* GET_CONF_NOTE:  <field> get_conf_note <value>
 *
 * Get a particular field from the header of the user's participation file
 * for the current conference.  Returns 0 if the item is not defined.
 */

void func_get_conf_note()
{
    char *field= pop_litname();
    Token val;

    if (pf_conf_get(field, &val))
	push_integer(0);
    else
    	push_token(&val,FALSE);

    free(field);
}


/* SET_CONF_NOTE:  <field> <value> set_conf_note -
 *
 * Set a particular field in the header of the user's participation file
 * for the current conference.  Value must be a string or an integer.
 */

void func_set_conf_note()
{
    Token val;
    char *field;

    pop_any(&val);
    field= pop_litname();

    if (type(val) == TK_INTEGER)
	pf_conf_seti(field, ival(val));
    else if (type(val) == TK_STRING)
	pf_conf_sets(field, sval(val), FALSE);
    else
	die("expected string or integer value");

    free(field);
}


/* GET_ITEM_NOTE:  <item> <field> get_conf_note <value>
 *
 * Get a particular field from the header of the user's participation file
 * for the current conference.  Returns 0 if the item is not defined.
 */

void func_get_item_note()
{
    char *field= pop_litname();
    int item= pop_integer();
    Token val;

    if (pf_item_get(item,field, &val))
	push_integer(0);
    else
    	push_token(&val,FALSE);

    free(field);
}


/* SET_ITEM_NOTE:  <item> <field> <value> get_item_note -
 *
 * Set a particular field for the given item in the user's participation file
 * for the current conference.  Value must be a string or an integer.
 */

void func_set_item_note()
{
    Token val;
    char *field;
    int item;

    pop_any(&val);
    field= pop_litname();
    item= pop_integer();

    if (type(val) == TK_INTEGER)
	pf_item_seti(item, field, ival(val));
    else if (type(val) == TK_STRING)
	pf_item_sets(item, field, sval(val), FALSE);
    else
	die("expected string or integer value");

    free(field);
}

/* FAV_SEL:  <isel> <flag> fav_first <isel>..
 * Rearrange an item selector so that the "favorite" and/or "brandnew" items
 * come first.  Flag is:
 *    0 = no change 
 *         returns <isel>
 *    1 = favorites first -
 *         returns <fav-isel> <others-isel>
 *    2 = brandnew first -
 *         returns <brandnew-isel> <newresp-isel>
 *    3 = favorites, then brandnew -
 *         returns <fav-isel> <brandnew-isel> <others-isel>
 */

void func_fav_sel()
{
    char *sel, *s[3];
    int n,i;
    int flag= pop_integer();

    if (flag == 0) return;
    if (flag < 0 || flag > 3) die("flag out of range");

    sel= pop_string();
    n= fav_sel(sel,flag,&s[0],&s[1],&s[2]);
    free(sel);

    for (i= 0; i < n; i++)
    {
	if (s[i] == NULL)
	    push_string("",TRUE);
	else
	    push_string(s[i],FALSE);
    }
}


#if ATTACHMENTS
#include "baaf.h"

/* MAKE_ATTACH:  <tmphandle> <desc> <resp> make_attach <handle>
 * 
 * Convert a temporary attachment handle into a permanent attachment handle.
 * File is moved, index entry is created.  There should be a currently open
 * conference.  Also uses the 'item', 'id' and 'uid' system variables.  If
 * desc is an empty string, the original name of the file is used as the
 * description.
 *
 * If resp is -1, the attachment is associated with the conference instead.
 * If resp is -2, the attachment is associated with the user instead.
 */

void func_make_attach()
{
    int rn= pop_integer();
    char *desc= pop_string();
    char *tmphand= pop_string();
    char *newhand;
    int access;
    
    if (rn < -2)
	die("Bad response number: %d",rn);
    if (rn > -2)
    {
	if (curr_conf[0] == '\0')
	    die("No conference currently open");

	/* How much protection does this attachment need? */
	if (bdict(VAR_USERLIST) || bdict(VAR_GROUPLIST) || bdict(VAR_SECRET))
	    access= 2;
	else if (bdict(VAR_ALLOWANON))
	    access= 0;
	else
	    access= 1;
    }

    newhand= make_attach(tmphand, desc,
	    (rn > -2) ? curr_conf : "",
	    (rn > -1) ? idict(VAR_ITEM) : 0,
	    rn,
	    access);

    push_string(newhand,FALSE);

    free(desc);
    free(tmphand);
}
#else
void func_make_attach() {func_pop(); func_pop();}	/* no-op */
#endif /* ATTACHMENTS */


/* MACROS:  The following functions are actually expanded out to different
 * things at compile time, so it would be really strange if they somehow
 * got into the program at run time.  So these are just dummy stubs to make
 * the compiler happy.
 */

void func_conf_bull() {}
void func_conf_index() {}
void func_conf_login() {}
void func_conf_logout() {}
