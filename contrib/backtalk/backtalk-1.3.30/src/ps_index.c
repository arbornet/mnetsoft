/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "ps_index.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* Item Index File Structure - One item index file is kept for every item in
 * every conference.  The item index file is a binary file consisting
 * of an array of structures like the one below.  The first one is for the
 * item text (response 0) and the successive ones are for the corresponding
 * successive items.
 *
 * Index File Notes:  Index files are never "trusted" files.  When we share
 * a database with picospan or yapp, they don't maintain the files.  Someone
 * may manually edit the item file, which could invalidate all the pointers
 * in the index file.  The index file could be truncated at some random point
 * by a full disk error.  These routines are supposed to be tolerant of this
 * kind of thing.  If the response requested is not indexed, we fall back to
 * searching the item file.  Any new information we pick up is added to the
 * index file.
 */

struct iindex {
    size_t offset;	/* Offset of the response in the main file */
    };

/* Macros to convert between response numbers and offsets into index file */
#define RESP_AT_OFFSET(o) (o / sizeof(struct iindex))
#define OFFSET_OF_RESP(r) (r*sizeof(struct iindex))


int curr_index_fd= -1;	/* file descriptor for index file for current item */
size_t curr_index_size;	/* size of current index file - -1 if unknown */


/* ITEM_INDEX_NAME - generate the name of the index file for the given
 * conference directory and item number.  Save it in the buffer bf.
 * If itemnumber is 0, return the full pathname of the indexdir.
 */

void item_index_name(char *bf, int bfsz, char *confdir, int itemnumber)
{
    if (strlen(confdir)+sizeof(INDEX_PREFIX)+sizeof(INDEX_SUBDIR)+10 > bfsz)
	die("conference directory path %s too long",confdir);

    if (itemnumber > 0)
	sprintf(bf,"%s/"INDEX_SUBDIR"/"INDEX_PREFIX"%d",confdir,itemnumber);
    else
	sprintf(bf,"%s/"INDEX_SUBDIR,confdir);
}


/* REMOVE_INDEX - delete the index file for an item.
 */

void remove_index(char *confdir, int itemnumber)
{
    char file[BFSZ];

    item_index_name(file,BFSZ,confdir,itemnumber);
    unlink(file);
}


/* LINK_INDEX - link the index file for that_item in that_conf, to an index
 * file for this_item in this_conf.
 */

void link_index(char *that_conf, int that_item, char *this_conf, int this_item)
{
    char that_file[BFSZ];
    char this_file[BFSZ];

    item_index_name(that_file,BFSZ,that_conf,that_item);
    item_index_name(this_file,BFSZ,this_conf,this_item);
    link(that_file,this_file);
}


/* CLOSE_ITEM_INDEX - Close any currently open item index file.
 */

void close_item_index()
{
    if (curr_index_fd < 0)
	return;

    close(curr_index_fd);
    curr_index_fd= -1;
}


/* OPEN_ITEM_INDEX - Open the item index file for the given item in the
 * given conference directory.  Close any previously open one first.
 */

void open_item_index(char *confdir, int itemnumber)
{
    char fname[BFSZ];

    close_item_index();

    item_index_name(fname, BFSZ, confdir, itemnumber);

    if ((curr_index_fd= open(fname,O_RDWR|O_CREAT,0644)) < 0 && 
	(itemnumber == 1 || (time(0L)%50) == 0))
    {
	/* We can run OK without an index file, but we are faster
	 * with it.  So we occasionally try creating an indexdir,
	 * in case that's what's missing.
	 */
	item_index_name(fname,BFSZ,confdir, 0);
	if (!mkdir(fname,0755))
	{
	    /* Mkdir succeeded - try opening the index file again */
	    item_index_name(fname, BFSZ, confdir, itemnumber);
	    curr_index_fd= open(fname,O_RDWR|O_CREAT,0644);
	}

    }
    curr_index_size= -1;
}


/* INDEX_NEW_RESP - Update the index file for an item to reflect that there
 * has been a response number rnum added at roffset.
 */

void index_new_resp(char *confdir, int inum, int rnum, size_t roffset)
{
    char file[BFSZ];
    struct iindex ii;

    /* Trap to try to find bug with occasional wild response numbers */
    if (rnum < 0 || rnum > 10000000)
	die("index_new_resp called with response number %d\n",rnum);

    open_item_index(confdir,inum);
    if (curr_index_fd < 0) return;

    ii.offset= roffset;

    lseek(curr_index_fd, OFFSET_OF_RESP(rnum), 0);
    write(curr_index_fd, &ii, sizeof(struct iindex));

    close_item_index();
}


/* SEEK_RESP -- Given item_fp, an open file descriptor for the current item
 * file, seek to response number resp.  It leaves item_fp pointing to the first
 * character AFTER the ,R that starts the response.  If resp is -1, seek to
 * the begining of the last response.  Returns the response number you sought
 * to, or -1 if the requested response does not exist.
 *
 * You should be careful to call open_item_index() before calling this.  This
 * does not automatically call it.  If the item index is not open, it just
 * works the old way - reading through the item to find the response.
 *
 * As a side effect, if this finds new responses that weren't in the index
 * file, it adds them in.
 */

int seek_resp(FILE *item_fp, int resp)
{
    int curr_resp, rebuild= 1;
    int c1,c2,c3;
    size_t index_offset, item_offset;
    size_t old_index_size;
    struct iindex ii;
    char t[3];

    ii.offset= 0L;

    /* Check if index file is open */
    if (curr_index_fd >= 0)
    {
	/* Search for response using the index file */

	/* First get size of index file (if we don't already have it) */
	if (curr_index_size == -1)
	{
	    struct stat st;
	    fstat(curr_index_fd, &st);
	    curr_index_size= st.st_size;
	}

	/* Check that index file is non-empty */
	if (curr_index_size >= sizeof(struct iindex))
	{
	    /* Find the nearest response and its location in the index file */
	    if (resp != -1)
	    {
		/* Response requested */
		curr_resp= resp;
		index_offset= OFFSET_OF_RESP(resp);
	    }
	    if (resp == -1 || index_offset >= curr_index_size)
	    {
		/* Last response in index file */
		curr_resp= RESP_AT_OFFSET(curr_index_size) - 1;
		index_offset= OFFSET_OF_RESP(curr_resp);
	    }

	    /* Read the item file offset from the index file */
	    lseek(curr_index_fd, index_offset, 0);
	    if (read(curr_index_fd, &ii, sizeof(struct iindex)) ==
	         sizeof(struct iindex) && ii.offset > 0L)
	    {
		item_offset= ii.offset;

		/* Check that there is really a response at that offset */
		fseek(item_fp, item_offset-1, 0);
		if (fread(t, 1, 3, item_fp) == 3 &&
		    t[0] == '\n' && t[1] == ',' && t[2] == 'R')
		{
		    /* This looks good then */
		    if (curr_resp == resp)
		    {
		        return resp;
		    }
		    rebuild= 0;
		}
	    }
	}
	old_index_size= curr_index_size;
	if (rebuild)
	{
	    /* Index file was empty or erroneous - do a total rebuild */
	    curr_resp= -1;
	    curr_index_size= 0L;
	    index_offset= 0L;
	    ii.offset= 0L;
	    item_offset= 0L;
	    lseek(curr_index_fd, 0L, 0);
	    fseek(item_fp, 0L, 0);
	}
    }
    else
    {
	/* Haven't got an index file */
	curr_resp= -1;
	item_offset= 0L;
	fseek(item_fp, 0L, 0);
	old_index_size= curr_index_size= 0L;
    }

    /* Index file either did nothing for us (rebuild == 1) or got us only
     * part way (rebuild == 0).  Need to read our way the rest of the way
     * through the item file, counting responses and writing out new index
     * file entries as we go.
     */

    c2= c3= '\0';

    for (;;)
    {
	c1= c2;  c2= c3;  c3= fgetc(item_fp);

	/* Have we found the end of the file? */
	if (c3 == EOF)
	{
	    /* Truncate the index file, if it got smaller */
	    if (curr_index_size < old_index_size)
	    	ftruncate(curr_index_fd, curr_index_size);

	    /* If we weren't looking for the end, we failed */
	    if (resp != -1 || ii.offset == 0L)
		return -1;

	    /* Found last response - Seek back to just after ,R */
	    fseek(item_fp, ii.offset+2, 0);
	    break;
	}

	item_offset++;

	/* Have we found the start of a response? */
	if (c1 == '\n' && c2 == ',' && c3 == 'R')
	{
	    /* Found start of a new response */
	    curr_resp++;
	    ii.offset= item_offset-2;

	    /* Write out new index file entry */
	    if (curr_index_fd >= 0)
	    {
		/* Make sure we did a seek between reading and writing */
		if (!rebuild)
		{
		     lseek(curr_index_fd,OFFSET_OF_RESP(curr_resp),0);
		     rebuild= 1;
		}
		write(curr_index_fd,&ii, sizeof(struct iindex));
		curr_index_size+= sizeof(struct iindex);
	    }

	    /* Have we found the response we were looking for? */
	    if (curr_resp == resp)
	    	break;
	}
    }

    return curr_resp;
}


/* REBUILD_INDEX - regenerate the index file for an item.  We do this in
 * place, to avoid losing any links that may exist to the file.
 */

void rebuild_index(FILE *item_fp, char *confdir, int itemnumber)
{
    /* Open the index file */
    open_item_index(confdir,itemnumber);
    if (curr_index_fd < 0) return;

    /* Empty the index file */
    ftruncate(curr_index_fd,0L);

    /* Do a seek to the end, which will rebuild the whole index */
    seek_resp(item_fp, -1);

    close_item_index();
}
