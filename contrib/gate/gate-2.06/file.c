#include "gate.h"
#include <pwd.h>


/* READ_FILE - Read the named file into our file.
 */

void read_file(char *rname, int strip)
{
    FILE *rfp;
    int nlines, nchars, nstrip;
    int och,ch;
    struct stat st;
	
    if (expand_tilde(rname)) return;

    if ((rfp= fopen(rname,"r")) == NULL)
    {
	printf("Cannot open file %s\n",rname);
	return;
    }
    fstat(fileno(rfp),&st);
    if ((st.st_mode&S_IFMT) == S_IFDIR)
    {
	printf("%s is a directory\n",rname);
	return;
    }

    nstrip= nlines= nchars= 0;
    while ((ch= fgetc(rfp)) != EOF)
    {
	if (!strip ||(isascii(ch) &&
		(isprint(ch) || ch == '\t' || ch == '\n' || ch == '\014')))
	{
	    nchars++;
	    if (ch == '\n') nlines++;

	    putc(ch,tfp);
	}
	else if (nstrip++ == 0)
	    printf("Stripping bad characters from file %s\n",rname);
	och= ch;
    }
    fclose(rfp);
    /* Add terminating newline, if there wasn't one */
    if (och != '\n')
    {
	putc('\n',tfp);
	nlines++;
    }
    printf("Included file %s (%d lines / %d characters)\n",rname,nlines,nchars);
    if (nstrip > 0)
       printf("%d characters stripped\n",nstrip);
}


/* COPY_FP - write a copy of our current buffer file into the open file with
 * the given descriptor.
 */

void copy_fp(FILE *wfp)
{
    int ch;

    fseek(tfp,0L,0);

    while ((ch= fgetc(tfp)) != EOF)
	putc(ch,wfp);
}


/* COPY_FILE - most of the guts of write_file - Create the named file,
 * write into it a copy of our current buffer file, and return the still
 * open file descriptor.
 */

FILE *copy_file(char *wname)
{
    FILE *wfp;
	
    if ((wfp= fopen(wname,"w+")) == NULL)
    {
	printf("Cannot open file %s\n",wname);
	return NULL;
    }

    copy_fp(wfp);
    return wfp;
}


/* WRITE_FILE - Write a copy of our file into the named file.
 */

void write_file(char *wname)
{
    FILE *fp;
  
    if (expand_tilde(wname)) return;

    if ((fp= copy_file(wname)) == NULL) return;
    fclose(fp);
    printf("Saved in file %s\n",wname);
}


/* TYPEFILE - Print contents of named file to screen, expanding out various
 * $-escapes as below:
 *    $$ -- print a $ sign
 *    $: -- print "cmdchar[0]"
 */

void typefile(char *name)
{
    int ch;
    FILE *fp;

    if ((fp= fopen(name,"r")) == NULL)
    {
	printf("Could not read file %s\n",name);
	return;
    }
    while ((ch= fgetc(fp)) != EOF)
    {
	if (ch == '$')
	    switch (ch= fgetc(fp))
	    {
	    case '$':
	    case EOF:
		putchar('$');
		break;
	    case ':':
		putchar(cmdchar[0]);
		break;
	    default:
		putchar('$');
		putchar(ch);
		break;
	    }
	else
	    putchar(ch);
    }
    fclose(fp);
}

/* EMPTYFILE - Empty the buffer file */

void emptyfile()
{
    fclose(tfp);
    if ((tfp= fopen(filename,"w+")) == NULL)
    {
	printf("Panic: can't reopen %s\n",filename);
	done(RET_ABORT);
    }
}


/* EXPAND_TILDE - This does ~ expansion in filenames.  It assume that you have
 * enough space in the text buffer to do the expansion in place.  Returns 1,
 * if expansion fails.
 */

int expand_tilde(char *name)
{
    char *expand, *rest;
    struct passwd *pw;
    int i,j;
    char oldname[BUFSIZE+2];

    if (name[0] == '~')
    {
	strcpy(oldname,name);

	if (oldname[1] == '/')
	{
	    expand= getenv("HOME");
	    rest= oldname + 1;
	}
	else 
	{
	    if ((rest= strchr(oldname,'/')) != NULL)
		*rest= '\0';

	    if ((pw= getpwnam(oldname+1)) == NULL)
	    {
		printf("Unknown user: %s\n",oldname+1);
		return 1;
	    }
	    if (rest != NULL) *rest= '/';

	    expand= pw->pw_dir;
	}

	strcpy(name,expand);
	if (rest != NULL)
	    strcat(name,rest);
    }
    return 0;
}


/* MV_FILE -- do a "mv" command -- both files must be on same file system.
 */

int mv_file(char *src, char *dst)
{
    if (link(src,dst))
    {
	printf("could not create %s\n",dst);
	return 1;
    }
    if (unlink(src))
    {
	printf("could not delete %s\n",src);
	return 1;
    }
    return 0;
}
