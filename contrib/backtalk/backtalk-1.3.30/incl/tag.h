/* Copyright 2004, Jan D. Wolter, All Rights Reserved. */ 

typedef struct
{
    char *buf;	/* Work area */
    char *p;	/* Pointer into work area */
} rtag;

typedef struct staglist {
    char *name;		/* Name of a tag */
    int type;		/* 0 = string, 1 = int */
    union {
	char *s;	/* String value of tag */
	int i;		/* Integer value of tag */
    } val;
    struct staglist *next;	/* Next tag */
} taglist;

rtag *open_tag(char *fname, int lock);
void close_tag(rtag *rt);
int get_tag(rtag *rt, char **var, char **val);
void wrtstrtag(FILE *fp, char *name, char *value, int protect);
void wrtinttag(FILE *fp, char *name, int value, int protect);
void wrttagline(FILE *fp, taglist *tag, int protect);
void unesctag(char *f, char *t, int proc);
void add_str_tag(taglist **tagl, char *name, char *val);
void add_int_tag(taglist **tagl, char *name, int val);
void update_tag(char *fname, taglist **tagl, int amadm);
