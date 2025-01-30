/* Copyright 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"
#include "stack.h"
#if ATTACHMENTS

struct commt {
    char *type;
    char *name;
} common_mime[]= {
    /* KEEP THESE ALPHABETICAL!! */
    {"application/msword",		"microsoft word"},
    {"application/octet-stream",	"binary"},
    {"application/pdf",			"pdf"},
    {"application/postscript",		"postscript"},
    {"application/rtf",			"rich text file"},
    {"application/x-gzip",		"gzip compressed data"},
    {"application/x-tar",		"tar archive"},
    {"audio/basic",			"audio"},
    {"audio/mpeg",			"mpeg audio"},
    {"audio/wav",			"windows audio"},
    {"audio/x-midi",			"midi audio"},
    {"image/gif",			"gif image"},
    {"image/jpeg",			"jpeg image"},
    {"image/png",			"png image"},
    {"text/html",			"html"},
    {"text/plain",			"plain text"},
    {"video/mpeg",			"mpeg video"},
    {"video/msvideo",			"avi video"},
    {"video/quicktime",			"quicktime video"},

    {NULL,				NULL}};

/* MIME_NAME:  Return an English name for a Mime type.  Some common ones are
 * hard coded, others are looked up in a file.  Returned name is in dynamic
 * memory.  If no name is found, just returns the value passed in.
 */

char *mime_name(char *type)
{
#ifdef MIMENAME_FILE
    FILE *fp;
    char bf[BFSZ], *t, *e;
#endif
    struct commt *p;
    int rc;

    /* First scan the in-memory table */
    /* If the in memory table gets big, a binary search or hash might be nice */
    for(p= common_mime; p->type != NULL; p++)
    {
	rc= strcmp(type,p->type);
	if (rc < 0) break;
	if (rc == 0) return strdup(p->name);
    }

#ifdef MIMENAME_FILE
    if ((fp= fopen(MIMENAME_FILE,"r")) == NULL)
	die("Could not open mimetype file: "MIMENAME_FILE);

    while (fgets(bf,BFSZ,fp) != NULL)
    {
	if (bf[0] == '#') continue;
	if ((t= strchr(bf,'\t')) == NULL) continue;
	*t= '\0';
	rc= strcmp(type,bf);
	if (rc < 0) break;
	if (rc == 0)
	{
	    /* Skip extra tabs */
	    for (t++; *t == '\t'; t++)
		;
	    if ((e= strchr(t,'\n')) != NULL) *e= '\0';
	    return strdup(t);
	}
    }
    fclose(fp);
#endif

    return strdup(type);
}

void func_mimename()
{
    char *type= peek_string();
    char *name= mime_name(type);
    free(type);
    repl_top((void *)name);
}

#else
/* If we don't have attachments, mimename is a no-op */
void func_mimename() {}
#endif /*ATTACHMENTS*/
