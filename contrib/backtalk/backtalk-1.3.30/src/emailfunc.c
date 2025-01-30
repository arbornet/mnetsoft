/* Copyright 2003, Jan D. Wolter, All Rights Reserved. */

#include <ctype.h>

#include "backtalk.h"
#include "email.h"
#include "stack.h"
#include "emailfunc.h"

#if EMAIL

static Email *curr_email= NULL;

/* BAD_ADDR - Check if an email address is in a sane form.  This is pretty
 * minimal right now.  We want printable characters, and one @ sign before
 * at least one dot.  Empty strings are OK.
 */

int bad_addr(char *addr)
{
    int at= FALSE, dot= FALSE;

    if (addr[0] == '\0') return FALSE;

    for (; *addr != '\0'; addr++)
    {
	if (!isascii(*addr) || !isprint(*addr))
	    return TRUE;
	if (at)
	{
	    if (*addr == '.') dot= TRUE;
	    if (*addr == '@') return TRUE;
	}
	else if (*addr == '@')
	    at= TRUE;
    }

    return !dot;
}


/* BAD_HEADER - Check if a string is legal header text - no newlines,
 * control characters or non-ascii stuff.  Empty strings are OK.
 */

int bad_header(char *str)
{
    int at= FALSE, dot= FALSE;

    for (; *str != '\0'; str++)
    {
	if (!isascii(*str) || !isprint(*str))
	    return TRUE;
    }
    return FALSE;
}


/* FUNC_EMAIL_NEW:   <subject> <reply-addr> <reply-name> email_new <rc>
 *
 * Start a new email message and make it the current email.  Any input may
 * be ().  Returns:
 *
 *   rc = 0     message created
 *   rc = 1     invalid subject
 *   rc = 2     invalid reply address
 *   rc = 3     invalid reply name
 */

void func_email_new()
{
    int rc;
    char *rep_name= pop_string();
    char *rep_addr= pop_string();
    char *subject=  pop_string();

    if (bad_header(subject))
	rc= 1;
    else if (bad_addr(rep_addr))
	rc= 2;
    else if (bad_header(rep_name))
	rc= 3;
    else
    {
	if (curr_email != NULL)
	    email_free(curr_email);

	curr_email= new_email(NULL, NULL,
		(subject[0] == '\0') ? NULL : subject,
		(rep_addr[0] == '\0') ? NULL : rep_addr,
		(rep_name[0] == '\0') ? NULL : rep_name);

	rc= 0;
    }
    push_integer(rc);
    free(rep_name);
    free(rep_addr);
    free(subject);
}


/* FUNC_EMAIL_BODY:   <alt> <desc> <type> <text>  email_body <rc>
 *
 *  Attach a body to the current email message.  <alt> can be 0 for the
 *  primary version of the text, or 1 for the alternate.  If there is already
 *  a body with the same value of <alt>, then this is an attachment.  <desc>
 *  is a description.  It can be ().  Type is any mime/type.  If a message
 *  has a primary body of type (text/html) and no alternate body, then an
 *  alternate body of type (text/plain) is automatically generated. Returns:
 *
 *   rc = 0     text added
 *   rc = 1     invalid description
 *   rc = 2     invalid type
 *
 *  Function dies on invalid alt values or if there is no current email.
 */

void func_email_body()
{
    int rc;
    char *text= pop_string();
    char *type= pop_string();
    char *desc= pop_string();
    int alt= pop_integer();

    if (curr_email == NULL)
	die("No current email message");
    else if (alt < 0 || alt > 1)
	die("Bad value for alt (%d)",alt);
    else if (bad_header(desc))
	rc= 1;
    else if (bad_header(type) || strchr(type,'/') == NULL)
	rc= 2;
    else
    {
	email_body(curr_email, alt, type, ENC_DUNNO, text, -1,
		(desc[0] == '\0') ? NULL : desc);
	rc= 0;
    }
    push_integer(rc);
    free(text);
    free(type);
    free(desc);
}


/* FUNC_EMAIL_RECIP:  <type> <addr> <name> email_recip <rc>
 *
 * Add a recipient to the current email message.  The recipient type may be
 * any of:
 *
 *   type = (T+)    append address to the "To" list
 *   type = (T)     set "To" list to contain only this address
 *   type = (C+)    append address to the "Cc" list
 *   type = (C)     set "Cc" list to contain only this address
 *   type = (B+)    append address to the "Bcc" list
 *   type = (B)     set "Bcc" list to contain only this address
 *
 * The address and name fields can be ().  Returns:
 *
 *   rc = 0     recipient added
 *   rc = 1     invalid address
 *   rc = 2     invalid name
 *
 *  Function dies on invalid type values or if there is no current email.
 */

void func_email_recip()
{
    int rc, recip;
    char *name= pop_string();
    char *addr= pop_string();
    char *type= pop_string();

    if (curr_email == NULL)
	die("No current email message");

    if (isascii(type[0]) && islower(type[0]))
	type[0]= toupper(type[0]);

    recip= -1;
    switch (type[0])
    {
	case 'T':  recip= REC_TO;  break;
	case 'C':  recip= REC_CC;  break;
	case 'B':  recip= REC_BCC; break;
    }

    if (recip == -1 || (type[1] != '+' && type[1] != '\0'))
	die("Bad value for recipient type (%s)",type);

    if (bad_addr(addr))
	rc= 1;
    else if (bad_header(name))
	rc= 2;
    else
    {
	email_recipient(curr_email, recip,
		(addr[0] == '\0') ? NULL : addr,
		(name[0] == '\0') ? NULL : name,
		type[1] == '+');
	rc= 0;
    }
    push_integer(rc);
    free(name);
    free(addr);
    free(type);
}


/* FUNC_EMAIL_SEND:  - email_send -
 *
 *  Send the current email.
 */

void func_email_send()
{
    int i;

    if (curr_email == NULL)
	die("No current email message");

    for (i= 0; i < N_REC; i++)
    {
	if (curr_email->to[i] != NULL) break;
    }
    if (i >= N_REC)
	die("No recipients on current email message");

    if (curr_email->cont == NULL)
	die("No content in current email message");

    if (i= email_send(curr_email))
	die("Email failed:  error code %d",i);
}


/* FUNC_EMAIL_CLOSE:   - email_close -
 *
 * Discard the current email, if there is one.
 */

void func_email_close()
{
    if (curr_email != NULL)
	email_free(curr_email);
    curr_email= NULL;
}

#else

/* No-Op versions for systems without email enabled */

void func_email_new()
{
    free(pop_string());
    free(pop_string());
    free(pop_string());
    push_integer(0);
}

void func_email_body()
{
    free(pop_string());
    free(pop_string());
    free(pop_string());
    (void) pop_integer();
    push_integer(0);
}

void func_email_recip()
{
    free(pop_string());
    free(pop_string());
    free(pop_string());
    push_integer(0);
}

void func_email_send() {}
void func_email_close() {}

#endif /* EMAIL */
