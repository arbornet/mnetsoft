/* Copyright 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Different kinds of recpients - passed to email_recipient() */

#define REC_TO	0	/* "To" line recipients */
#define REC_CC	1	/* "Cc" line recipients */
#define REC_BCC	2	/* "Bcc" line recipients */
#define N_REC   3

/* Different kinds of content encoding - passed to email_body() */

#define ENC_DUNNO -1	/* Unknown - let the software choose the best */
#define ENC_NONE 0	/* Don't encode */
#define ENC_QP   1	/* Use "quoted-printable" quoting */
#define ENC_B64  2	/* Use "base64" quoting */

/* output format options - passed to email_write() */

#define EFG_CRNL     1	/* Terminate lines with CRNL instead of NL */
#define EFG_BCC      2	/* Write out the "Bcc" header line */
#define EFG_DOTSPACE 4	/* Append a space to any line containing just a dot */
#define EFG_DOTTWICE 8  /* Double any dot at beginning of a line */

/* Structure for an email address linked list */

struct emaddr {
    char *addr;		    /* Email address of person */
    char *name;		    /* Full name of person */
    struct emaddr *next;
};

/* Structure for a message content linked list */

struct emcont {
    char *text[2];	    /* prime and alternate versions of text */
    int tlen[2];	    /* length of prime and alternate versions of text */
    char *desc[2];	    /* prime and alternate content-description text */
    char *type[2];	    /* types of texts (like text/html) */
    char etype[2];	    /* type of encoding */
    struct emcont *next;
};

/* Structure for a message */

typedef struct {
    char *subj;		       /* Subject line */
    struct emaddr from;	       /* From email address and name */
    struct emaddr replyto;     /* Email address to reply to */
    struct emaddr *to[N_REC];  /* To, Cc, and Bcc lists */
    struct emaddr *tol[N_REC]; /* Last entries on To, Cc and Bcc lists */
    struct emcont *cont;       /* Content list */
    struct emcont *contl;      /* Last entry on content list */
    char encoded;	       /* has body been encoded? */
    char *boundary;	       /* Boundary being used for this message */
} Email;


Email *new_email(char *fromaddr, char *fromname, char *subj, char *replyaddr,
	char *replyname);
void email_recipient(Email *email, int type, char *addr, char *name,
	int append);
void email_body(Email *email, int alt, char *contype, int enctype,
	        char *text, int len, char *desc);
void email_free(Email *email);
void email_write(Email *email, int flags, FILE *fp);
int email_send(Email *email);
