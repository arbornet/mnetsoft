/* (c) 1996-2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

typedef struct vstring {
    char *begin;	/* base of allocated memory */
    size_t size;	/* currently allocated amount of memory */
    char *p;		/* current pointer */
    char *last;		/* spare pointer into text if not NULL */
} VSTRING;

/* VS_NCAT - Append n characters from the given string to a VSTRING */
#define vs_cat_n(vs,str,len) memmove(vs_inc_n(vs,len),str,len)

int vs_new(VSTRING *vs, size_t size);
int vs_start(VSTRING *vs, size_t size, char *val, int len);
void vs_empty(VSTRING *vs);
void vs_destroy(VSTRING *vs);
int vs_realloc(VSTRING *vs, size_t size);
void vs_more(VSTRING *vs);
char *vs_inc(VSTRING *vs);
char *vs_inc_n(VSTRING *vs, size_t n);
void vs_cat(VSTRING *vs, char *psz);
char *vs_return(VSTRING *vs);
