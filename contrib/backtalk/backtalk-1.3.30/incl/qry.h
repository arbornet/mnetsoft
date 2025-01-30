/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

struct sqlbuf {
    char *bf;		/* Pointer to base of buffer */
    int sz;		/* size of buffer */
    int i;		/* offset to current end of buffer */
    };

void makeqry(struct sqlbuf *b);
#if __STDC__
void addqry(struct sqlbuf *b, const char *fmt, ...);
#else
void addqry();
#endif
void addquoteqry(struct sqlbuf *b, char *str);
char *finalqry(struct sqlbuf *b);
void resetqry(struct sqlbuf *b);
void freeqry(struct sqlbuf *b);
