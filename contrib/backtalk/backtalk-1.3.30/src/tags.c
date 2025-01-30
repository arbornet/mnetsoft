/* Table of Interesting HTML Tags
 *
 * Tags in this table get some processing from Backtalk.  All tags not in this
 * are automatically deleted by format_html().  Some of the tags in this table
 * are also deleted.
 *
 * Table must be alphabetical.
 */

#include <stdio.h>
#include "tags.h"

#ifdef MK_TAGS
#define T(n,l,c,b,h,i) {n,l,c,b,h,-1}
#else
#include "tagdefs.h"
#define T(n,l,c,b,h,i) {n,l,c,b,h,i}
#endif

struct tagt tagtype[] =
{
    /*           LEGAL CLOSE BRKLN HRDSP INSIDE */

    /* !DOCTYPE    banned - effects whole document */
    T("A",	   1,    1,    0,    0,  -1),
    T("ABBR",	   1,    1,    0,    0,  -1),
    T("ACRONYM",   1,    1,    0,    0,  -1),
    T("ADDRESS",   1,    1,    0,    0,  -1),
    /* APPLET      banned - introduce executable content */
    T("AREA",	   1,    0,    0,    0,  Tag_MAP),
    T("B",	   1,    1,    0,    0,  -1),
    T("BANNER",	   1,    1,    0,    0,  -1),
    /* BASE        banned - effects whole document */
    /* BASEFONT    banned - effects whole document */
    T("BDO",	   1,    1,    0,    0,  -1),
    /* BGSOUND     banned - no noises, thank you */
    T("BIG",	   1,    1,    0,    0,  -1),
    /* BLINK       banned - too ugly */
    T("BLOCKQUOTE",1,    1,    1,    0,  -1),
    /* BODY        banned - effects whole document */
    T("BR",	   1,    0,    1,    0,  -1),
    /* BUTTON      banned - no forms */
    T("CAPTION",   1,    1,    0,    0,  Tag_TABLE),
    T("CENTER",	   1,    1,    0,    0,  -1),
    T("CITE",	   1,    1,    1,    0,  -1),
    T("CODE",	   1,    1,    0,    0,  -1),
    T("COL",	   1,    0,    0,    0,  Tag_TABLE),
    T("COLGROUP",  1,    0,    0,    0,  Tag_TABLE),
    /* COMMENT     banned - IE weirdness */
    T("DD",	   1,    0,    1,    0,  -1),
    T("DEL",	   1,    1,    0,    0,  -1),
    T("DFN",	   1,    1,    0,    0,  -1),
    T("DIR",	   1,    1,    1,    0,  -1),
    T("DIV",	   1,    1,    1,    0,  -1),
    T("DL",	   1,    1,    1,    0,  -1),
    T("DT",	   1,    0,    1,    0,  -1),
    T("EM",	   1,    1,    0,    0,  -1),
    /* EMBED       banned - introduces executable content */
    /* FIELDSET    banned - no forms */
    T("FONT",	   1,    1,    0,    0,  -1),
    /* FORM        banned - no forms */
    /* FRAME       banned - effects whole document */
    /* FRAMESET    banned - effects whole document */
    T("H1",	   1,    1,    1,    0,  -1),
    T("H2",	   1,    1,    1,    0,  -1),
    T("H3",	   1,    1,    1,    0,  -1),
    T("H4",	   1,    1,    1,    0,  -1),
    T("H5",	   1,    1,    1,    0,  -1),
    T("H6",	   1,    1,    1,    0,  -1),
    /* HEAD        banned - effects whole document */
    /* HR          banned - used as response separator */
    /* HTML        banned - effects whole document */
    T("I",	   1,    1,    0,    0,  -1),
    /* IFRAME      banned - effects whole document */
    /* ILAYER      banned - effects whole document */
    T("IMG",	   1,    0,    0,    0,  -1),
    /* INPUT       banned - no forms */
    T("INS",	   1,    1,    0,    0,  -1),
    /* ISINDEX     banned - effects whole document */
    T("KBD",	   1,    1,    1,    0,  -1),
    /* KEYGEN      banned - no forms */
    /* LABEL       banned - no forms */
    /* LAYER       banned - effects whole document */
    /* LEGEND      banned - no forms */
    T("LI",	   1,    0,    1,    0,  -1),
    /* LINK        banned - effects whole document */
    T("LISTING",   1,    1,    0,    1,  -1),
    T("MAP",	   1,    1,    0,    0,  -1),
    T("MENU",	   1,    1,    1,    0,  -1),
    /* META        banned - effects whole document */
    T("MULTICOL",  1,    1,    1,    0,  -1),
    T("NOBR",	   1,    1,    0,    0,  -1),
    /* NOEMBED     banned - we never do embeds */
    /* NOFRAMES    banned - we never do frames */
    /* NOSCRIPT    banned - we never do scripts */
    /* OBJECT      banned - introduces executable content */
    T("OL",	   1,    1,    1,    0,  -1),
    /* OPTGROUP    banned - no forms */
    /* OPTION      banned - no forms */
    T("P",	   1,    0,    0,    0,  -1),
    /* PARAM       banned - introduces executable content */
    /* PLAINTEXT   banned - effects whole document */
    T("PRE",	   1,    1,    0,    1,  -1),
    T("Q",	   1,    1,    0,    0,  -1),
    T("S",	   1,    1,    0,    0,  -1),
    T("SAMP",	   1,    1,    0,    0,  -1),
    T("SCRIPT",	   0,    1,    0,    1,  -1),
    /* SELECT      banned - no forms */
    /* SERVER      banned - introduces executable content */
    T("SMALL",	   1,    1,    0,    0,  -1),
    T("SPACER",	   1,    0,    0,    0,  -1),
    T("SPAN",	   1,    1,    0,    0,  -1),
    T("STRIKE",	   1,    1,    0,    0,  -1),
    T("STRONG",	   1,    1,    0,    0,  -1),
    /* STYLE       banned - effects whole document */
    T("SUB",	   1,    1,    0,    0,  -1),
    T("SUP",	   1,    1,    0,    0,  -1),
    T("TABLE",	   1,    1,    1,    0,  -1),
    T("TBODY",	   1,    1,    0,    0,  Tag_TABLE),
    T("TD",	   1,    1,    1,    0,  Tag_TR),
    /* TEXTAREA    banned - no forms */
    T("TFOOT",	   1,    1,    0,    0,  Tag_TABLE),
    T("TH",	   1,    1,    0,    0,  Tag_TABLE),
    T("THEAD",	   1,    1,    0,    0,  Tag_TABLE),
    T("TITLE",	   0,    1,    1,    0,  -1),
    T("TR",	   1,    1,    1,    0,  Tag_TABLE),
    T("TT",	   1,    1,    0,    0,  -1),
    T("U",	   1,    1,    0,    0,  -1),
    T("UL",	   1,    1,    1,    0,  -1),
    T("VAR",	   1,    1,    0,    0,  -1),
    T("WBR",	   1,    0,    0,    0,  -1),
    T("XMP",	   1,    1,    0,    1,  -1),

    T(NULL,	   0,    0,    0,    1,  -1)
};
