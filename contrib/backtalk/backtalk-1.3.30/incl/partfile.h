/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* PARTICIPATION FILE - Normally one participation file is kept in memory.
 * It is for the conference in the sysdict variable "conf" and has been
 * read if "part_loaded" is true.
 *
 * "part_modified" is true if any change has been made to the data stored in
 * a PicoSpan-style participation file.  "ext_modified" is true if any of the
 * data added by Backtalk has been changed.  If either has been changed, we
 * should write the new one out before exiting.
 *
 *
 * The participation file is stored in a linked list.
 *
 * Note that we hold the participation file open between loading it and
 * writing it, so that our lock on it doesn't go away.
 */

/* Structure for user/item information */

struct pfe {
    int item_no;		/* Item number */
    int resp_no;		/* Number of responses read including item */
    time_t  date;		/* Time of last reading */
    char *my_title;		/* User's own title for the item */
    char forgotten;		/* Is this item forgotten? */
    char favorite;		/* Is this item a favorite? */
    char delete;		/* Should this entry not be written out? */
    HashTable *etc;		/* Other random fields */
    struct pfe *next;		/* Pointer to next entry */
    };

/* Structure of user/conf information */

struct pfc {
    int part_type;		/* What type of particip. file was it?	 */
				/*   0= Picospan only (never happens)    */
				/*   1= Picospan plus extension file     */
				/*   2= Backtalk                         */

    int part_modified;		/* Need to write out part file when done? */
    int ext_modified;		/* Need to write out extensions when done? */

    int snooping;		/* Is conference open for snooping only? */

    time_t part_date;		/* Last mod date - -1=dunno, 0=brandnew */
    FILE *part_fp;		/* File descriptor for participation file */
    FILE *ext_fp;		/* File descriptor for extension file */
    char part_name[BFSZ];	/* fullpath name of current particip. file */
    char *alias;		/* User's full name, as in particip. file */
    HashTable *conf_etc;	/* Miscellaneous conference fields */
    struct pfe *pflist;         /* List of items */
    struct pfe *last_part;	/* Last pflist item looked at */
    char *conf;			/* Name of conference (sometimes) */
    char *confdir;		/* Directory of conference (sometimes) */
    char *user;			/* ID of user in (sometimes) */
};

int loadpart(struct pfc *part);
void etc_int(HashTable **ht, char *tag, int value);
void etc_str(HashTable **ht, char *tag, char *value, int copy);
int find_pf(struct pfc *part, int item, int create, struct pfe **pf);
void newpart(struct pfc *part, int newuser);
void writepart(struct pfc *part);
void freepart(struct pfc *part);
char *homepartpath(char *path, char *dir, char *name);
int openpf(struct pfc *part);
void closepf(struct pfc *part);
int openpfext(struct pfc *part);
#ifdef PARTUTIL
int partutil(char *flag, char *conf, char *user);
#endif /* PARTUTIL */
