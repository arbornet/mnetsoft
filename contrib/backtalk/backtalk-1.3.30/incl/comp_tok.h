/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Data structure used to compile program into -- basically a token with lots
 * of links.  The child pointer is hidden in the t.val field of tokens with
 * type TK_PROCEDURE.
 */

struct ctoken {
	Token t;                /* The Token */
	struct ctoken *parent;  /* The parent PROC token */
	struct ctoken *next;    /* The next token */
	struct ctoken *prev;    /* The previous token */
	};
typedef struct ctoken CToken;

void init_comp(int sfid);
CToken *add_tok(word flag, byte file, word line, void *val);
void add_ctoken(CToken *new);
void start_proc(byte file, word line);
int end_proc(void);
int proc_len(CToken *p);
void write_token(int fd, CToken *p);
int write_prog(int fd);
void do_inline(int cmd, char *filename, byte file, word line);
void free_ctoken(CToken *p);
void drop_token(int n, int free, byte *file, word *line);
void drop_program(void);
void add_proc(CToken *p, int free);
int template(char *tmpl, ...);
void dump_prog(void);

#define add_token(t) add_tok((t)->flag,(t)->sfid,(t)->line,(t)->val)
