/* Copyright 2003, Jan D. Wolter, All Rights Reserved. */

#include <stdio.h>
#include "common.h"
#include "email.h"

main()
{
    Email *e= new_email("jan@unixpapa.com","Jan Wolter","This is a test",
	    "reply@unixpapa.com","Reply Wolter");
    email_recipient(e, REC_TO, "janc", "Jan Recipient Wolter", TRUE);
    email_body(e, 1, "text/plain", ENC_DUNNO,
	    "--==--===--==__==--===--==--==--==--\n.\n.onedot\nThis is a test message.\nIt has many lines.\nThe quick brown fox jumped over the lazy dog.  I hope you are getting this, Helen.\n--==--===--==__==--===--==--==--\n", -1,"The liney part");
    /*email_body(e, 0, "text/html", ENC_QP,
	    "<STRONG>This is a test message</STRONG>.\nIt has many lines.\nThe quick brown fox jumped over the lazy dog.  I <EM>hope</EM> you are getting this, Helen.\n", -1, "The html part");
    email_body(e, 0, "text/html", ENC_QP,
	    "<STRONG>STRONG</STRONG>.\nwide and fat.\n", -1, "The wide and fat part"); */
    /*email_write(e,EFG_BCC|EFG_DOTSPACE,stdout);*/
    email_send(e);
    email_free(e);
}
