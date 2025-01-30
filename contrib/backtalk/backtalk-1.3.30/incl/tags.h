/* (c) 1996-2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

extern struct tagt {
    char *name;			/* Name of tag */
    char legal;			/* True if legal - if banned delete content */
    char close;			/* Does tag require a closing tag? */
    char brkln;			/* Does tag break line? */
    char hrdsp;			/* Is white space significant in tag? */
    int inside;			/* What tag must this be enclosed in? */
    } tagtype[];
