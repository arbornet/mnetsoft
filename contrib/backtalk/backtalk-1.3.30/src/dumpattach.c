#include "common.h"
#include "baaf_core.h"
#include "baai.h"
#include "vstr.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif



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


main(int argc,char **argv)
{
    int next= FALSE;
    char *handle,*type,*user,*conf,*desc,*ds;
    int link, access;
    off_t size;
    uid_t uid;
    time_t date;

    date= time(0L);
    ds= ctime(&date);
    ds[24]= '\0';
    printf("<attachindex date=\"%ld\" cdate=\"%s\" version=\"%d.%d.%d\">\n",
	    date, ds, VERSION_A, VERSION_B, VERSION_C);

    while (baai_walk(next,&handle,&link,&type,&size,&user,&uid,&conf,&access,
	&date,&desc))
    {
	ds= ctime(&date);
	ds[24]= '\0';
	printf(" <attach handle=\"%s\"\n  link=\"%d\"\n  type=\"%s\"\n"
	       "  size=\"%ld\"\n  user=\"%s\"\n  uid=\"%d\"\n"
	       "  conf=\"%s\"\n  access=\"%d\"\n  date=\"%d\""
	       " cdate=\"%s\"\n  desc=\"%s\" />\n",
	       xmlquote(handle),link,xmlquote(type),(long)size,xmlquote(user),
	       uid,xmlquote(conf),access,date,ds,xmlquote(desc));
	free(handle);
	free(type);
	free(user);
	free(conf);
	free(desc);
	next= TRUE;
    }
    printf("</attachindex>\n");

    return 0;
}
